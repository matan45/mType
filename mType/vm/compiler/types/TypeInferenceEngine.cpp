#include "TypeInferenceEngine.hpp"
#include <cstddef>
#include "../../../types/TypeConversionUtils.hpp"
#include "../../../ast/nodes/expressions/IntegerNode.hpp"
#include "../../../ast/nodes/expressions/FloatNode.hpp"
#include "../../../ast/nodes/expressions/StringNode.hpp"
#include "../../../ast/nodes/expressions/BoolNode.hpp"
#include "../../../ast/nodes/expressions/NullNode.hpp"
#include "../../../ast/nodes/expressions/VariableNode.hpp"
#include "../../../ast/nodes/expressions/BinaryExpNode.hpp"
#include "../../../ast/nodes/expressions/UnaryExpNode.hpp"
#include "../../../ast/nodes/expressions/CastExpression.hpp"
#include "../../../ast/nodes/expressions/LambdaNode.hpp"
#include "../../../ast/nodes/expressions/AwaitExpression.hpp"
#include "../../../ast/nodes/expressions/IndexAccessNode.hpp"
#include "../../../ast/nodes/expressions/ArrayLiteralNode.hpp"
#include "../../../ast/nodes/expressions/ArrayCreationNode.hpp"
#include "../../../ast/nodes/expressions/TernaryExpNode.hpp"
#include "../../../ast/nodes/classes/NewNode.hpp"
#include "../../../ast/nodes/classes/MemberAccessNode.hpp"
#include "../../../ast/nodes/classes/MethodCallNode.hpp"
#include "../../../ast/nodes/classes/FieldNode.hpp"
#include "../../../ast/nodes/functions/FunctionCallNode.hpp"
#include "../../../token/TokenType.hpp"

namespace vm::compiler::types
{
    TypeInferenceEngine::TypeInferenceEngine(
        const bytecode::BytecodeProgram& program,
        std::shared_ptr<environment::Environment> environment,
        const variables::VariableTracker& variableTracker,
        const variables::GlobalVariableRegistry& globalRegistry
    )
        : program(program)
        , environment(environment)
        , variableTracker(variableTracker)
        , globalRegistry(globalRegistry)
    {
    }

    bool TypeInferenceEngine::isEffectivelyNullLiteral(ast::ASTNode* node)
    {
        while (auto* cast = dynamic_cast<ast::CastExpression*>(node))
        {
            node = cast->getExpression();
        }
        return dynamic_cast<ast::NullNode*>(node) != nullptr;
    }

    value::ValueType TypeInferenceEngine::inferLiteralType(ast::ASTNode* node) const
    {
        if (dynamic_cast<ast::IntegerNode*>(node)) return value::ValueType::INT;
        if (dynamic_cast<ast::FloatNode*>(node)) return value::ValueType::FLOAT;
        if (dynamic_cast<ast::StringNode*>(node)) return value::ValueType::STRING;
        if (dynamic_cast<ast::BoolNode*>(node)) return value::ValueType::BOOL;
        if (dynamic_cast<ast::NullNode*>(node)) return value::ValueType::OBJECT;
        if (dynamic_cast<ast::NewNode*>(node)) return value::ValueType::OBJECT;
        if (dynamic_cast<ast::LambdaNode*>(node)) return value::ValueType::OBJECT;
        if (dynamic_cast<ast::nodes::expressions::ArrayLiteralNode*>(node)) return value::ValueType::ARRAY;
        // MYT-282: `new T[N]` parses as ArrayCreationNode (distinct from
        // ArrayLiteralNode). Pre-fix this fell through to VOID and method-arg
        // type checks failed with "expects Object but got void".
        if (dynamic_cast<ast::nodes::expressions::ArrayCreationNode*>(node)) return value::ValueType::ARRAY;
        return value::ValueType::VOID;
    }

    std::shared_ptr<runtimeTypes::klass::FieldDefinition>
    TypeInferenceEngine::resolveQualifiedStaticField(const std::string& varName) const
    {
        size_t pos = varName.find("::");
        if (pos == std::string::npos || !environment) {
            return nullptr;
        }

        std::string className = varName.substr(0, pos);
        std::string fieldName = varName.substr(pos + 2);

        auto classDef = environment->getClassRegistry()->findClass(className);
        if (!classDef) {
            return nullptr;
        }

        return classDef->getField(fieldName);
    }

    value::ValueType TypeInferenceEngine::normalizeFieldValueType(
        value::ValueType type, const std::string& typeName) const
    {
        if (type == value::ValueType::ARRAY) {
            return value::ValueType::ARRAY;
        }

        if (!typeName.empty() &&
            (typeName.find("[]") != std::string::npos || typeName.find("Array<") == 0)) {
            return value::ValueType::ARRAY;
        }

        if (type == value::ValueType::OBJECT && environment && !environment->findClass(typeName)) {
            if (typeName == "Int") return value::ValueType::INT;
            if (typeName == "Float") return value::ValueType::FLOAT;
            if (typeName == "Bool") return value::ValueType::BOOL;
            if (typeName == "String") return value::ValueType::STRING;
        }

        return type;
    }

    value::ValueType TypeInferenceEngine::inferVariableType(ast::VariableNode* varNode) const
    {
        std::string varName = varNode->getName();

        // MYT-377: `Class::FIELD` parses into a single VariableNode whose name is
        // the qualified string. Resolve the static field's declared type so typed
        // parameter/assignment positions infer the real type instead of VOID.
        if (varName.find("::") != std::string::npos) {
            if (auto field = resolveQualifiedStaticField(varName)) {
                auto unifiedType = field->getUnifiedType();
                return normalizeFieldValueType(
                    field->getType(),
                    unifiedType ? unifiedType->toString() : "");
            }
            return value::ValueType::VOID;
        }

        const auto& locals = variableTracker.getLocals();
        for (auto it = locals.rbegin(); it != locals.rend(); ++it) {
            if (it->name == varName) {
                if (!it->className.empty() && it->className.back() == ']') {
                    return value::ValueType::ARRAY;
                }
                return it->type;
            }
        }

        if (globalRegistry.exists(varName)) {
            std::string className = globalRegistry.getClassName(varName);
            if (!className.empty() && className.back() == ']') {
                return value::ValueType::ARRAY;
            }
            return globalRegistry.getType(varName);
        }

        if (currentClassNode && inInstanceMethod) {
            for (const auto& field : currentClassNode->getFields()) {
                if (auto* fieldNode = dynamic_cast<ast::FieldNode*>(field.get())) {
                    if (fieldNode->getName() == varName && !fieldNode->getIsStatic()) {
                        auto genericType = fieldNode->getGenericType();
                        return normalizeFieldValueType(
                            fieldNode->getType(),
                            genericType ? genericType->toString() : "");
                    }
                }
            }

            if (environment && currentClassNode->hasParentClass()) {
                auto parentDef = environment->getClassRegistry()->findClass(currentClassNode->getParentClassName());
                while (parentDef) {
                    auto field = parentDef->getField(varName);
                    if (field && !field->isStatic() &&
                        field->getAccessModifier() != ast::AccessModifier::PRIVATE) {
                        auto unifiedType = field->getUnifiedType();
                        return normalizeFieldValueType(
                            field->getType(),
                            unifiedType ? unifiedType->toString() : "");
                    }

                    parentDef = parentDef->getParentClass();
                }
            }
        }

        return value::ValueType::VOID;
    }

    value::ValueType TypeInferenceEngine::inferUnaryOperationType(ast::UnaryOpNode* unaryOp) const
    {
        auto operandType = inferExpressionType(unaryOp->getOperand());
        auto op = unaryOp->getOperator();

        if (op == token::TokenType::MINUS || op == token::TokenType::PLUS) {
            if (operandType == value::ValueType::INT || operandType == value::ValueType::FLOAT) {
                return operandType;
            }
        }

        if (op == token::TokenType::NOT) {
            return value::ValueType::BOOL;
        }

        return operandType;
    }

    value::ValueType TypeInferenceEngine::inferCastType(ast::CastExpression* castExpr) const
    {
        const auto* targetType = castExpr->getTargetType();
        if (targetType) {
            std::string targetTypeName = targetType->toString();
            if (targetTypeName == "int") return value::ValueType::INT;
            if (targetTypeName == "float") return value::ValueType::FLOAT;
            if (targetTypeName == "string") return value::ValueType::STRING;
            if (targetTypeName == "bool") return value::ValueType::BOOL;
            return value::ValueType::OBJECT;
        }
        return value::ValueType::VOID;
    }

    value::ValueType TypeInferenceEngine::inferMemberAccessType(ast::MemberAccessNode* memberAccess) const
    {
        std::string className = inferExpressionClassName(memberAccess->getObject());
        if (!className.empty()) {
            auto classDef = environment->findClass(className);
            if (classDef) {
                std::string memberName = memberAccess->getMemberName();

                auto field = classDef->getField(memberName);
                if (field) {
                    if (field->getType() == value::ValueType::ARRAY) {
                        return value::ValueType::ARRAY;
                    }

                    if (field->hasUnifiedType()) {
                        std::string fieldTypeName = field->getUnifiedType()->toString();
                        if (!fieldTypeName.empty() &&
                            (fieldTypeName.find("[]") != std::string::npos || fieldTypeName.find("Array<") == 0)) {
                            return value::ValueType::ARRAY;
                        }
                    }
                    return field->getType();
                }

                auto method = classDef->getMethod(memberName);
                if (method) {
                    if (method->getReturnType() == value::ValueType::ARRAY) {
                        return value::ValueType::ARRAY;
                    }

                    if (method->getUnifiedReturnType()) {
                        std::string returnTypeName = method->getUnifiedReturnType()->toString();
                        if (!returnTypeName.empty() &&
                            (returnTypeName.find("[]") != std::string::npos || returnTypeName.find("Array<") == 0)) {
                            return value::ValueType::ARRAY;
                        }
                    }
                    return method->getReturnType();
                }
            }
        }
        return value::ValueType::VOID;
    }

    value::ValueType TypeInferenceEngine::inferIndexAccessType(ast::nodes::expressions::IndexAccessNode* indexAccess) const
    {
        std::string collectionClassName = inferExpressionClassName(indexAccess->getCollection());

        std::string elementType;

        if (collectionClassName.find("Array<") == 0)
        {
            size_t start = collectionClassName.find('<');
            size_t end = collectionClassName.rfind('>');
            if (start != std::string::npos && end != std::string::npos && end > start)
            {
                elementType = collectionClassName.substr(start + 1, end - start - 1);
            }
        }
        else if (!collectionClassName.empty() && collectionClassName.length() >= 2 &&
                 collectionClassName.substr(collectionClassName.length() - 2) == "[]")
        {
            elementType = collectionClassName.substr(0, collectionClassName.length() - 2);
        }

        if (!elementType.empty())
        {
            if (elementType == "int") return value::ValueType::INT;
            if (elementType == "float") return value::ValueType::FLOAT;
            if (elementType == "string") return value::ValueType::STRING;
            if (elementType == "bool") return value::ValueType::BOOL;
            return value::ValueType::OBJECT;
        }

        return value::ValueType::VOID;
    }

    value::ValueType TypeInferenceEngine::inferTernaryType(
        ast::nodes::expressions::TernaryExpNode* ternary) const
    {
        value::ValueType trueType = inferExpressionType(ternary->getTrueExpression());
        value::ValueType falseType = inferExpressionType(ternary->getFalseExpression());

        if (trueType == falseType) {
            return trueType;
        }

        return value::ValueType::VOID;
    }

    value::ValueType TypeInferenceEngine::inferExpressionType(ast::ASTNode* node) const
    {
        if (!node) return value::ValueType::VOID;

        value::ValueType literalType = inferLiteralType(node);
        if (literalType != value::ValueType::VOID) {
            return literalType;
        }

        if (auto* varNode = dynamic_cast<ast::VariableNode*>(node)) {
            return inferVariableType(varNode);
        }

        if (auto* funcCall = dynamic_cast<ast::FunctionCallNode*>(node)) {
            return inferFunctionCallType(funcCall);
        }

        if (auto* unaryOp = dynamic_cast<ast::UnaryOpNode*>(node)) {
            return inferUnaryOperationType(unaryOp);
        }

        if (auto* castExpr = dynamic_cast<ast::CastExpression*>(node)) {
            return inferCastType(castExpr);
        }

        if (auto* memberAccess = dynamic_cast<ast::MemberAccessNode*>(node)) {
            return inferMemberAccessType(memberAccess);
        }

        if (auto* methodCall = dynamic_cast<ast::MethodCallNode*>(node)) {
            return inferMethodCallType(methodCall);
        }

        if (auto* binOp = dynamic_cast<ast::BinaryOpNode*>(node)) {
            return inferBinaryOperationType(binOp);
        }

        if (auto* indexAccess = dynamic_cast<ast::nodes::expressions::IndexAccessNode*>(node)) {
            return inferIndexAccessType(indexAccess);
        }

        if (auto* ternary = dynamic_cast<ast::nodes::expressions::TernaryExpNode*>(node)) {
            return inferTernaryType(ternary);
        }

        // await: async fns return Promise<T> where T is a wrapper class, so the
        // unwrapped value is still OBJECT. Return inner type for non-OBJECT (rare).
        if (auto* awaitExpr = dynamic_cast<ast::nodes::expressions::AwaitExpression*>(node)) {
            auto* innerExpr = awaitExpr->getExpressionPtr();
            if (innerExpr) {
                auto innerType = inferExpressionType(innerExpr);
                if (innerType == value::ValueType::OBJECT) {
                    return value::ValueType::OBJECT;
                }
                return innerType;
            }
        }

        return value::ValueType::VOID;
    }

    void TypeInferenceEngine::setGenericTypeBindingsStack(
        const std::vector<std::unordered_map<std::string, std::string>>* stack)
    {
        genericTypeBindingsStack = stack;
    }

    void TypeInferenceEngine::setResolvedFunctionCallTypes(
        const std::unordered_map<const ast::ASTNode*, std::string>* cache)
    {
        resolvedFunctionCallTypes = cache;
    }

    void TypeInferenceEngine::setCurrentClassContext(ast::ClassNode* classNode, bool instanceMethod)
    {
        currentClassNode = classNode;
        inInstanceMethod = instanceMethod;
    }

    void TypeInferenceEngine::setNullNarrowingTracker(const NullNarrowingTracker* tracker)
    {
        nullNarrowingTracker = tracker;
    }

    std::string TypeInferenceEngine::resolveGenericType(const std::string& typeName) const
    {
        if (!genericTypeBindingsStack || genericTypeBindingsStack->empty())
        {
            return typeName;
        }

        for (auto it = genericTypeBindingsStack->rbegin(); it != genericTypeBindingsStack->rend(); ++it)
        {
            auto found = it->find(typeName);
            if (found != it->end())
            {
                return found->second;
            }
        }

        return typeName;
    }
}
