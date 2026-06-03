#include "AnnotationConstantResolver.hpp"

#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <variant>
#include <vector>

#include "../constant_folding/ConstantEvaluator.hpp"

#include "../../../errors/TypeException.hpp"
#include "../../../value/ValueType.hpp"

#include "../../../ast/GenericType.hpp"
#include "../../../ast/nodes/statements/ProgramNode.hpp"
#include "../../../ast/nodes/statements/BlockNode.hpp"
#include "../../../ast/nodes/statements/ImportNode.hpp"
#include "../../../ast/nodes/classes/ClassNode.hpp"
#include "../../../ast/nodes/classes/InterfaceNode.hpp"
#include "../../../ast/nodes/classes/FieldNode.hpp"
#include "../../../ast/nodes/classes/MethodNode.hpp"
#include "../../../ast/nodes/classes/ConstructorNode.hpp"
#include "../../../ast/nodes/functions/FunctionNode.hpp"
#include "../../../ast/nodes/annotations/AnnotationNode.hpp"
#include "../../../ast/nodes/annotations/AnnotationDeclarationNode.hpp"
#include "../../../ast/nodes/annotations/TypedAnnotationValue.hpp"
#include "../../../ast/nodes/expressions/IntegerNode.hpp"
#include "../../../ast/nodes/expressions/FloatNode.hpp"
#include "../../../ast/nodes/expressions/BoolNode.hpp"
#include "../../../ast/nodes/expressions/StringNode.hpp"
#include "../../../ast/nodes/expressions/NullNode.hpp"
#include "../../../ast/nodes/expressions/BinaryExpNode.hpp"
#include "../../../ast/nodes/expressions/UnaryExpNode.hpp"
#include "../../../ast/nodes/expressions/TernaryExpNode.hpp"
#include "../../../ast/nodes/expressions/CastExpression.hpp"
#include "../../../ast/nodes/expressions/VariableNode.hpp"
#include "../../../ast/nodes/expressions/ArrayLiteralNode.hpp"

namespace optimizer::passes::annotation_folding
{
    namespace
    {
        namespace expr = ast::nodes::expressions;
        namespace cls = ast::nodes::classes;
        namespace stmt = ast::nodes::statements;
        namespace ann = ast::nodes::annotations;
        using ann::TypedAnnotationValue;
        using ann::AnnotationValueType;
        using value::Value;
        using errors::TypeException;
        using errors::SourceLocation;
        using constant_folding::ConstantEvaluator;

        // Split "Class::Field" into ("Class", "Field") on the LAST "::".
        // Returns false when there is no scope qualifier.
        bool splitQualified(const std::string& qualified, std::string& cls, std::string& field)
        {
            auto pos = qualified.rfind("::");
            if (pos == std::string::npos) return false;
            cls = qualified.substr(0, pos);
            field = qualified.substr(pos + 2);
            return true;
        }

        enum class ConstState { UNRESOLVED, IN_PROGRESS, RESOLVED, FAILED };

        struct ConstEntry
        {
            cls::FieldNode* field = nullptr;
            ConstState state = ConstState::UNRESOLVED;
            std::optional<Value> value;
        };

        class Resolver
        {
        public:
            void run(ast::ASTNode* root)
            {
                collectConstants(root);
                resolveAnnotations(root);
            }

        private:
            std::unordered_map<std::string, ConstEntry> constants_;
            std::unordered_set<std::string> allFields_; // every "Class::field" (any modifier)
            std::vector<std::string> resolveStack_; // for cyclic-dependency messages

            // ---- Phase 1: gather candidate static-final constants ----------
            void collectConstants(ast::ASTNode* node)
            {
                if (!node) return;

                if (auto* program = dynamic_cast<stmt::ProgramNode*>(node))
                {
                    for (const auto& s : program->getStatements()) collectConstants(s.get());
                    return;
                }
                if (auto* block = dynamic_cast<stmt::BlockNode*>(node))
                {
                    for (const auto& s : block->getStatements()) collectConstants(s.get());
                    return;
                }
                if (auto* import = dynamic_cast<stmt::ImportNode*>(node))
                {
                    if (import->isResolved() && import->getImportedAST())
                        collectConstants(import->getImportedAST());
                    return;
                }
                if (auto* classNode = dynamic_cast<cls::ClassNode*>(node))
                {
                    for (const auto& f : classNode->getFields())
                    {
                        auto* field = dynamic_cast<cls::FieldNode*>(f.get());
                        if (!field) continue;
                        const std::string key = classNode->getClassName() + "::" + field->getName();
                        allFields_.insert(key);
                        if (field->getIsStatic() && field->getIsFinal() && field->hasInitialValue())
                        {
                            // First declaration wins (matches class-registry semantics).
                            constants_.try_emplace(key, ConstEntry{field, ConstState::UNRESOLVED, std::nullopt});
                        }
                    }
                    return;
                }
            }

            Value resolveConst(const std::string& qualified, const SourceLocation& loc)
            {
                auto it = constants_.find(qualified);
                if (it == constants_.end())
                {
                    std::string className, fieldName;
                    splitQualified(qualified, className, fieldName);
                    if (allFields_.count(qualified))
                    {
                        // Field exists but is not a usable compile-time constant.
                        throw TypeException(
                            "Annotation constant '" + qualified + "' must reference a 'static final' "
                            "field; '" + fieldName + "' on '" + className + "' is not static final (or "
                            "has no initializer).",
                            loc);
                    }
                    throw TypeException(
                        "Cannot resolve annotation constant '" + qualified +
                        "' — no static final field '" + fieldName + "' found on class '" + className + "'.",
                        loc);
                }

                ConstEntry& entry = it->second;
                switch (entry.state)
                {
                case ConstState::RESOLVED:
                    return *entry.value;
                case ConstState::IN_PROGRESS:
                {
                    std::string chain;
                    for (const auto& n : resolveStack_) chain += n + " -> ";
                    chain += qualified;
                    throw TypeException(
                        "Cyclic compile-time constant dependency detected while resolving '" + chain + "'.",
                        loc);
                }
                case ConstState::FAILED:
                    throw TypeException(
                        "Static final field '" + qualified + "' is not a compile-time constant, so it "
                        "cannot be used as an annotation argument.",
                        loc);
                case ConstState::UNRESOLVED:
                    break;
                }

                entry.state = ConstState::IN_PROGRESS;
                resolveStack_.push_back(qualified);
                // Inside a constant's initializer, bare identifiers resolve
                // against the constant's own class (e.g. `static final B = A * 2`).
                std::string ownerClass, ownerField;
                splitQualified(qualified, ownerClass, ownerField);
                Value v;
                try
                {
                    v = foldExpression(entry.field->getInitialValue(),
                                       entry.field->getLocation(), ownerClass);
                }
                catch (...)
                {
                    // Preserve the precise inner diagnosis (cyclic dependency,
                    // non-constant initializer, etc.).
                    entry.state = ConstState::FAILED;
                    resolveStack_.pop_back();
                    throw;
                }
                entry.value = v;
                entry.state = ConstState::RESOLVED;
                resolveStack_.pop_back();
                return v;
            }

            // ---- Recursive constant-expression folder ----------------------
            // `currentClass` is the enclosing class when folding a constant's
            // own initializer (so bare identifiers resolve to Class::name);
            // empty for annotation arguments (where bare identifiers are class
            // references, not constants, and therefore rejected).
            Value foldExpression(ast::ASTNode* node, const SourceLocation& loc,
                                 const std::string& currentClass = std::string())
            {
                if (!node)
                    throw TypeException("Annotation argument expression is empty.", loc);

                if (auto* n = dynamic_cast<expr::IntegerNode*>(node)) return Value(n->getValue());
                if (auto* n = dynamic_cast<expr::FloatNode*>(node))   return Value(n->getValue());
                if (auto* n = dynamic_cast<expr::BoolNode*>(node))    return Value(n->getValue());
                if (auto* n = dynamic_cast<expr::StringNode*>(node))  return Value(n->getValue());
                if (dynamic_cast<expr::NullNode*>(node))              return Value(std::monostate{});

                if (auto* bin = dynamic_cast<expr::BinaryExpNode*>(node))
                {
                    Value l = foldExpression(bin->getLeft(), loc, currentClass);
                    Value r = foldExpression(bin->getRight(), loc, currentClass);
                    auto res = ConstantEvaluator::evaluateBinaryOp(l, r, bin->getOperator());
                    if (!res)
                        throw TypeException(
                            "Annotation argument expression could not be folded to a constant "
                            "(overflow, division by zero, or unsupported operand types).",
                            loc);
                    return *res;
                }

                if (auto* un = dynamic_cast<expr::UnaryExpNode*>(node))
                {
                    if (un->getOperator() == token::TokenType::INCREMENT ||
                        un->getOperator() == token::TokenType::DECREMENT)
                    {
                        throw TypeException(
                            "Annotation argument expressions may only use literal constants and "
                            "'Class::FIELD' references; increment/decrement is not allowed.",
                            loc);
                    }
                    Value operand = foldExpression(un->getOperand(), loc, currentClass);
                    auto res = ConstantEvaluator::evaluateUnaryOp(operand, un->getOperator());
                    if (!res)
                        throw TypeException(
                            "Annotation argument expression could not be folded to a constant.",
                            loc);
                    return *res;
                }

                if (auto* tern = dynamic_cast<expr::TernaryExpNode*>(node))
                {
                    Value cond = foldExpression(tern->getCondition(), loc, currentClass);
                    if (!value::isBool(cond))
                        throw TypeException(
                            "Annotation ternary condition must fold to a boolean constant.", loc);
                    return value::asBool(cond)
                        ? foldExpression(tern->getTrueExpression(), loc, currentClass)
                        : foldExpression(tern->getFalseExpression(), loc, currentClass);
                }

                if (auto* cast = dynamic_cast<expr::CastExpression*>(node))
                {
                    Value inner = foldExpression(cast->getExpression(), loc, currentClass);
                    const ast::GenericType* target = cast->getTargetType();
                    value::ValueType vt;
                    if (!target || !primitiveCastType(target->getBaseTypeName(), vt))
                        throw TypeException(
                            "Annotation argument cast must target a primitive type (int, float, bool, String).",
                            loc);
                    auto res = ConstantEvaluator::evaluateTypeCast(inner, vt);
                    if (!res)
                        throw TypeException(
                            "Annotation argument cast could not be folded to a constant.", loc);
                    return *res;
                }

                if (auto* var = dynamic_cast<expr::VariableNode*>(node))
                {
                    const std::string& name = var->getName();
                    if (name.find("::") != std::string::npos)
                        return resolveConst(name, loc);
                    // Bare identifier inside a constant's initializer → resolve
                    // against the enclosing class (e.g. `static final B = A * 2`).
                    if (!currentClass.empty() &&
                        constants_.count(currentClass + "::" + name))
                        return resolveConst(currentClass + "::" + name, loc);
                    throw TypeException(
                        "Bare identifier '" + name + "' is treated as a Class reference; use "
                        "'ClassName::" + name + "' to reference a static final constant.",
                        loc);
                }

                throw TypeException(
                    "Annotation argument is not a compile-time constant: only literal constant "
                    "expressions and 'Class::FIELD' references are allowed (no 'new', method calls, "
                    "or instance access).",
                    loc);
            }

            static bool primitiveCastType(const std::string& baseName, value::ValueType& out)
            {
                if (baseName == "int")    { out = value::ValueType::INT;    return true; }
                if (baseName == "float")  { out = value::ValueType::FLOAT;  return true; }
                if (baseName == "bool")   { out = value::ValueType::BOOL;   return true; }
                if (baseName == "String") { out = value::ValueType::STRING; return true; }
                return false;
            }

            TypedAnnotationValue valueToTyped(const Value& v, const SourceLocation& loc)
            {
                if (value::isInt(v))       return TypedAnnotationValue::makeInt(value::asInt(v));
                if (value::isFloat(v))     return TypedAnnotationValue::makeFloat(value::asFloat(v));
                if (value::isBool(v))      return TypedAnnotationValue::makeBool(value::asBool(v));
                if (value::isAnyString(v)) return TypedAnnotationValue::makeString(std::string(value::asStringView(v)));
                if (value::isNullType(v) || value::isVoid(v)) return TypedAnnotationValue::makeNull();
                throw TypeException(
                    "Annotation argument did not fold to a supported constant value "
                    "(int, float, bool, string, or null).",
                    loc);
            }

            // Fold a deferred ArrayLiteralNode into a homogeneous typed array,
            // preserving the int[] -> float[] promotion rule.
            TypedAnnotationValue foldArray(expr::ArrayLiteralNode* arr, const SourceLocation& loc)
            {
                std::vector<Value> vals;
                vals.reserve(arr->getElements().size());
                for (const auto& e : arr->getElements())
                    vals.push_back(foldExpression(e.get(), loc));

                if (vals.empty()) return TypedAnnotationValue::makeClassArray({});

                bool allInt = true, allNumeric = true, allString = true, allBool = true;
                for (const auto& v : vals)
                {
                    allInt     = allInt     && value::isInt(v);
                    allNumeric = allNumeric && (value::isInt(v) || value::isFloat(v));
                    allString  = allString  && value::isAnyString(v);
                    allBool    = allBool    && value::isBool(v);
                }

                if (allInt)
                {
                    std::vector<int64_t> out;
                    for (const auto& v : vals) out.push_back(value::asInt(v));
                    return TypedAnnotationValue::makeIntArray(std::move(out));
                }
                if (allNumeric)
                {
                    std::vector<double> out;
                    for (const auto& v : vals)
                        out.push_back(value::isFloat(v) ? value::asFloat(v)
                                                        : static_cast<double>(value::asInt(v)));
                    return TypedAnnotationValue::makeFloatArray(std::move(out));
                }
                if (allString)
                {
                    std::vector<std::string> out;
                    for (const auto& v : vals) out.push_back(std::string(value::asStringView(v)));
                    return TypedAnnotationValue::makeStringArray(std::move(out));
                }
                if (allBool)
                {
                    std::vector<bool> out;
                    for (const auto& v : vals) out.push_back(value::asBool(v));
                    return TypedAnnotationValue::makeBoolArray(std::move(out));
                }

                throw TypeException(
                    "Annotation array element type mismatch: all elements must fold to the same "
                    "primitive type (int/float, string, or bool).",
                    loc);
            }

            // ---- Phase 2: fold every deferred annotation expression --------
            void foldAnnotation(ann::AnnotationNode& annotation)
            {
                if (!annotation.hasDeferredExpressions()) return;
                for (const auto& [key, exprNode] : annotation.getDeferredExpressions())
                {
                    const SourceLocation loc = exprNode ? exprNode->getLocation() : annotation.getLocation();
                    if (auto* arr = dynamic_cast<expr::ArrayLiteralNode*>(exprNode.get()))
                    {
                        annotation.setTypedParameter(key, foldArray(arr, loc));
                    }
                    else
                    {
                        Value v = foldExpression(exprNode.get(), loc);
                        annotation.setTypedParameter(key, valueToTyped(v, loc));
                    }
                }
                annotation.clearDeferredExpressions();
            }

            void foldAll(const std::vector<std::shared_ptr<ann::AnnotationNode>>& list)
            {
                for (const auto& a : list)
                    if (a) foldAnnotation(*a);
            }

            void foldParamAnnotations(
                const std::vector<std::vector<std::shared_ptr<ann::AnnotationNode>>>& perParam)
            {
                for (const auto& group : perParam) foldAll(group);
            }

            void resolveAnnotations(ast::ASTNode* node)
            {
                if (!node) return;

                if (auto* program = dynamic_cast<stmt::ProgramNode*>(node))
                {
                    for (const auto& s : program->getStatements()) resolveAnnotations(s.get());
                    return;
                }
                if (auto* block = dynamic_cast<stmt::BlockNode*>(node))
                {
                    for (const auto& s : block->getStatements()) resolveAnnotations(s.get());
                    return;
                }
                if (auto* import = dynamic_cast<stmt::ImportNode*>(node))
                {
                    if (import->isResolved() && import->getImportedAST())
                        resolveAnnotations(import->getImportedAST());
                    return;
                }
                if (auto* classNode = dynamic_cast<cls::ClassNode*>(node))
                {
                    foldAll(classNode->getAnnotations());
                    for (const auto& f : classNode->getFields())
                        if (auto* field = dynamic_cast<cls::FieldNode*>(f.get()))
                            foldAll(field->getAnnotations());
                    for (const auto& m : classNode->getMethods())
                        if (auto* method = dynamic_cast<cls::MethodNode*>(m.get()))
                        {
                            foldAll(method->getAnnotations());
                            foldParamAnnotations(method->getParameterAnnotations());
                        }
                    for (const auto& c : classNode->getConstructors())
                        if (auto* ctor = dynamic_cast<cls::ConstructorNode*>(c.get()))
                        {
                            foldAll(ctor->getAnnotations());
                            foldParamAnnotations(ctor->getParameterAnnotations());
                        }
                    return;
                }
                if (auto* interfaceNode = dynamic_cast<cls::InterfaceNode*>(node))
                {
                    for (const auto& m : interfaceNode->getMethods())
                        if (auto* method = dynamic_cast<cls::MethodNode*>(m.get()))
                        {
                            foldAll(method->getAnnotations());
                            foldParamAnnotations(method->getParameterAnnotations());
                        }
                    return;
                }
                if (auto* function = dynamic_cast<ast::nodes::functions::FunctionNode*>(node))
                {
                    foldAll(function->getAnnotations());
                    foldParamAnnotations(function->getParameterAnnotations());
                    return;
                }
                if (auto* annDecl = dynamic_cast<ann::AnnotationDeclarationNode*>(node))
                {
                    foldAll(annDecl->getMetaAnnotations());
                    return;
                }
            }
        };
    }

    void AnnotationConstantResolver::resolve(ast::ASTNode* root)
    {
        if (!root) return;
        Resolver resolver;
        resolver.run(root);
    }
}
