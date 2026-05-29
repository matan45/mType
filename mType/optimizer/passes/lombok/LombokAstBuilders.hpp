#pragma once

// Shared tiny AST-node builders for the Lombok synthesis codegen. Mirrors the
// anonymous-namespace helper style of StructuralEqualityCodegen, but exposed as
// `inline` functions in a detail namespace so the several Lombok codegen
// translation units (getters/setters/toString, constructors, builder) can share
// them without ODR violations.

#include "../../../ast/AccessModifier.hpp"
#include "../../../ast/GenericType.hpp"
#include "../../../ast/nodes/annotations/AnnotationNode.hpp"
#include "../../../ast/nodes/classes/ClassNode.hpp"
#include "../../../ast/nodes/classes/FieldNode.hpp"
#include "../../../ast/nodes/classes/MemberAccessNode.hpp"
#include "../../../ast/nodes/classes/MethodCallNode.hpp"
#include "../../../ast/nodes/expressions/StringNode.hpp"
#include "../../../ast/nodes/expressions/VariableNode.hpp"
#include "../../../ast/nodes/expressions/NullNode.hpp"
#include "../../../ast/nodes/expressions/TernaryExpNode.hpp"
#include "../../../ast/nodes/functions/ReturnNode.hpp"
#include "../../../ast/nodes/expressions/BinaryExpNode.hpp"
#include "../../../ast/nodes/statements/BlockNode.hpp"
#include "../../../ast/nodes/statements/MemberAssignmentNode.hpp"
#include "../../../token/TokenType.hpp"
#include "../../../value/ParameterType.hpp"
#include "../../../value/ValueType.hpp"
#include "../shared/AccessorNaming.hpp"

#include <memory>
#include <string>
#include <vector>

namespace optimizer::passes::lombok::detail
{
    using namespace ast;
    using namespace ast::nodes::expressions;
    using namespace ast::nodes::statements;
    using namespace ast::nodes::classes;
    using namespace ast::nodes::functions;
    using token::TokenType;
    using value::ValueType;

    // ---- name helpers -----------------------------------------------------
    // Re-export the shared accessor naming (optimizer::passes::shared) so the
    // compiler-side codegen and the LSP code action stay byte-compatible.
    using shared::capitalize;
    using shared::getterName;
    using shared::setterName;

    // ---- annotation / accessibility helpers -------------------------------

    // True iff the field carries a `@<name>` annotation (MYT-108 per-field
    // annotations), e.g. `@Getter private int x;`.
    inline bool fieldHasAnnotation(const FieldNode* field, const std::string& name)
    {
        for (const auto& ann : field->getAnnotations())
        {
            if (ann && ann->getName() == name) return true;
        }
        return false;
    }

    // True iff an INHERITED field can be read via `this.<field>` from a
    // subclass body. Private parent fields cannot (runtime access violation),
    // so they must be excluded from a subclass's synthesized toString. Own
    // fields are always readable and are not gated by this.
    inline bool isInheritedFieldAccessible(const FieldNode* field)
    {
        return field->getAccessModifier() != AccessModifier::PRIVATE;
    }

    // ---- expression builders ---------------------------------------------

    inline std::unique_ptr<ASTNode> makeThis()
    {
        return std::make_unique<VariableNode>("this");
    }

    inline std::unique_ptr<ASTNode> makeVar(const std::string& name)
    {
        return std::make_unique<VariableNode>(name);
    }

    inline std::unique_ptr<ASTNode> makeThisField(const std::string& fieldName)
    {
        return std::make_unique<MemberAccessNode>(makeThis(), fieldName, /*isStatic*/ false);
    }

    inline std::unique_ptr<ASTNode> makeStringLit(const std::string& value)
    {
        return std::make_unique<StringNode>(value);
    }

    inline std::unique_ptr<ASTNode> makeBinaryPlus(std::unique_ptr<ASTNode> lhs,
                                                   std::unique_ptr<ASTNode> rhs)
    {
        return std::make_unique<BinaryExpNode>(std::move(lhs), TokenType::PLUS, std::move(rhs));
    }

    inline std::unique_ptr<ASTNode> makeMethodCall0(std::unique_ptr<ASTNode> receiver,
                                                    const std::string& methodName)
    {
        std::vector<std::unique_ptr<ASTNode>> args;
        return std::make_unique<MethodCallNode>(
            std::move(receiver), methodName, std::move(args), /*isStatic*/ false);
    }

    inline std::unique_ptr<ASTNode> makeReturn(std::unique_ptr<ASTNode> expr)
    {
        return std::make_unique<ReturnNode>(std::move(expr));
    }

    inline std::unique_ptr<ASTNode> makeNull()
    {
        return std::make_unique<NullNode>();
    }

    // `<expr> == null`
    inline std::unique_ptr<ASTNode> makeIsNull(std::unique_ptr<ASTNode> expr)
    {
        return std::make_unique<BinaryExpNode>(std::move(expr), TokenType::EQUALS, makeNull());
    }

    // `<cond> ? <whenTrue> : <whenFalse>`
    inline std::unique_ptr<ASTNode> makeTernary(std::unique_ptr<ASTNode> cond,
                                                std::unique_ptr<ASTNode> whenTrue,
                                                std::unique_ptr<ASTNode> whenFalse)
    {
        return std::make_unique<TernaryExpNode>(
            std::move(cond), std::move(whenTrue), std::move(whenFalse));
    }

    // `this.<field> = <value>;`
    inline std::unique_ptr<ASTNode> makeThisFieldAssign(const std::string& fieldName,
                                                        std::unique_ptr<ASTNode> value)
    {
        return std::make_unique<MemberAssignmentNode>(makeThis(), fieldName, std::move(value));
    }

    inline std::shared_ptr<ASTNode> makeBlock(std::vector<std::unique_ptr<ASTNode>> stmts)
    {
        auto block = std::make_shared<BlockNode>(std::move(stmts));
        return std::static_pointer_cast<ASTNode>(block);
    }

    // ---- type helpers -----------------------------------------------------

    // Clones a field's declared GenericType so it can be reused as a method
    // return type or parameter type without sharing ownership with the field.
    inline std::shared_ptr<GenericType> cloneFieldType(const FieldNode* field)
    {
        auto gt = field->getGenericType();
        if (!gt) return std::make_shared<GenericType>(ValueType::OBJECT);
        return std::make_shared<GenericType>(*gt);
    }

    // Maps a field's GenericType to a constructor ParameterType. Mirrors the
    // compiler's ClassRegistrar::resolveMethodParameterType so synthesized
    // constructor parameters match what normal call-site argument resolution
    // produces. Note an array field (e.g. `int[]`) is stored as a
    // string-variant GenericType whose base name is "Array", so it lands in
    // the isGenericParameter() branch and MUST be detected by its "[]" suffix
    // — otherwise it would be mistaken for a class named "Array".
    inline value::ParameterType fieldToParameterType(const FieldNode* field)
    {
        auto gt = field->getGenericType();
        if (!gt) return value::ParameterType(ValueType::OBJECT);

        if (gt->isGenericParameter())
        {
            // toString() preserves array brackets and generic args
            // (e.g. "int[]", "Holder<Int>").
            std::string typeName = gt->toString();
            if (typeName.size() >= 2 &&
                typeName.compare(typeName.size() - 2, 2, "[]") == 0)
            {
                return value::ParameterType::forArray(typeName);
            }
            // Class or interface name. The optimizer can't consult the class/
            // interface registries (they aren't populated until compile), so
            // we default to forClass — correct for class-typed fields;
            // interface-typed fields are a known gap.
            return value::ParameterType::forClass(typeName);
        }

        ValueType vt = gt->getConcreteType();
        if (vt == ValueType::ARRAY)
        {
            return value::ParameterType::forArray(gt->toString());
        }
        return value::ParameterType(vt);
    }

    // True for concrete primitive field types that mType implicitly coerces in
    // string concatenation (`"x=" + intField`). Object/array/parameterized
    // fields need an explicit `.toString()` call instead.
    inline bool isStringConcatPrimitive(const FieldNode* field)
    {
        auto gt = field->getGenericType();
        if (!gt) return false;
        if (gt->isGenericParameter()) return false;       // class/interface name
        if (gt->isParameterized()) return false;           // Array<T>, etc.
        ValueType vt = gt->getConcreteType();
        return vt == ValueType::INT || vt == ValueType::FLOAT ||
               vt == ValueType::BOOL || vt == ValueType::STRING;
    }
}
