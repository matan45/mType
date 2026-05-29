#pragma once

// Shared tiny AST-node builders for the Lombok synthesis codegen. Mirrors the
// anonymous-namespace helper style of StructuralEqualityCodegen, but exposed as
// `inline` functions in a detail namespace so the several Lombok codegen
// translation units (getters/setters/toString, constructors, builder) can share
// them without ODR violations.

#include "../../../ast/GenericType.hpp"
#include "../../../ast/nodes/classes/ClassNode.hpp"
#include "../../../ast/nodes/classes/FieldNode.hpp"
#include "../../../ast/nodes/classes/MemberAccessNode.hpp"
#include "../../../ast/nodes/classes/MethodCallNode.hpp"
#include "../../../ast/nodes/expressions/StringNode.hpp"
#include "../../../ast/nodes/expressions/VariableNode.hpp"
#include "../../../ast/nodes/functions/ReturnNode.hpp"
#include "../../../ast/nodes/expressions/BinaryExpNode.hpp"
#include "../../../ast/nodes/statements/BlockNode.hpp"
#include "../../../ast/nodes/statements/MemberAssignmentNode.hpp"
#include "../../../token/TokenType.hpp"
#include "../../../value/ParameterType.hpp"
#include "../../../value/ValueType.hpp"

#include <cctype>
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

    // "name" -> "Name"; empty string stays empty.
    inline std::string capitalize(const std::string& s)
    {
        if (s.empty()) return s;
        std::string out = s;
        out[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(out[0])));
        return out;
    }

    inline std::string getterName(const std::string& field) { return "get" + capitalize(field); }
    inline std::string setterName(const std::string& field) { return "set" + capitalize(field); }

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

    // Maps a field's GenericType to a constructor ParameterType, mirroring the
    // parser's own GenericType -> ParameterType mapping (ParameterParser.cpp).
    // A class-typed field is stored as a string-variant GenericType, so
    // isGenericParameter() is the "named type" branch here (generic classes
    // are skipped upstream, so a bare type-parameter never reaches this).
    inline value::ParameterType fieldToParameterType(const FieldNode* field)
    {
        auto gt = field->getGenericType();
        if (!gt) return value::ParameterType(ValueType::OBJECT);

        value::ParameterType pt = [&]() {
            if (gt->isGenericParameter())
            {
                return value::ParameterType::forClass(gt->getGenericName());
            }
            ValueType vt = gt->getConcreteType();
            if (vt == ValueType::ARRAY)
            {
                return value::ParameterType::forArray(gt->toString());
            }
            return value::ParameterType(vt);
        }();
        pt.nullable = gt->isNullable();
        return pt;
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
