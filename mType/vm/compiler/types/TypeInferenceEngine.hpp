#pragma once
#include "../../../ast/ASTNode.hpp"
#include <cstddef>
#include "../../../ast/nodes/classes/ClassNode.hpp"
#include "../../../value/ValueType.hpp"
#include "../../bytecode/BytecodeProgram.hpp"
#include "../../../environment/Environment.hpp"
#include "../variables/VariableTracker.hpp"
#include "../variables/GlobalVariableRegistry.hpp"
#include "../../../token/TokenType.hpp"
#include "NullNarrowingTracker.hpp"
#include <string>
#include <memory>
#include <unordered_map>
#include <vector>

namespace ast::nodes::expressions
{
    class ArrayCreationNode;
    class ArrayLiteralNode;
    class AwaitExpression;
}

namespace vm::compiler::types
{
    /**
     * Performs type inference for expressions
     * Determines the ValueType and class name for any expression node
     */
    class TypeInferenceEngine
    {
    public:
        TypeInferenceEngine(
            const bytecode::BytecodeProgram& program,
            std::shared_ptr<environment::Environment> environment,
            const variables::VariableTracker& variableTracker,
            const variables::GlobalVariableRegistry& globalRegistry
        );

        ~TypeInferenceEngine() = default;

        // Type inference methods
        value::ValueType inferExpressionType(ast::ASTNode* node) const;
        std::string inferExpressionClassName(ast::ASTNode* node) const;

        // Nullability inference - returns true if expression may evaluate to null
        bool inferExpressionNullable(ast::ASTNode* node) const;

        // Returns true if the expression is the null literal, possibly wrapped in
        // any number of cast expressions. Casts cannot launder null into a non-
        // nullable target — null-assignment validation must see through them.
        static bool isEffectivelyNullLiteral(ast::ASTNode* node);

        // Set null narrowing tracker for smart cast awareness
        void setNullNarrowingTracker(const NullNarrowingTracker* tracker);

        // Set generic type bindings stack (reference to context's stack)
        void setGenericTypeBindingsStack(const std::vector<std::unordered_map<std::string, std::string>>* stack);

        // PHASE 3: Set reference to resolved function call types cache
        void setResolvedFunctionCallTypes(const std::unordered_map<const ast::ASTNode*, std::string>* cache);

        // Set current class context for field type inference
        void setCurrentClassContext(ast::ClassNode* classNode, bool isInstanceMethod);

    private:
        const bytecode::BytecodeProgram& program;
        std::shared_ptr<environment::Environment> environment;
        const variables::VariableTracker& variableTracker;
        const variables::GlobalVariableRegistry& globalRegistry;
        const NullNarrowingTracker* nullNarrowingTracker = nullptr;
        const std::vector<std::unordered_map<std::string, std::string>>* genericTypeBindingsStack = nullptr;
        const std::unordered_map<const ast::ASTNode*, std::string>* resolvedFunctionCallTypes = nullptr;  // PHASE 3
        ast::ClassNode* currentClassNode = nullptr;  // Current class context for field lookup
        bool inInstanceMethod = false;  // Whether we're in an instance method

        // Helper to resolve generic types
        std::string resolveGenericType(const std::string& typeName) const;

        // Resolves overloaded function/method metadata by name. Tries an exact
        // lookup first, then a prefix match against mangled names of the form
        // "callName/<paramTypes>", scoring candidates by argument arity and
        // parameter-type compatibility. Returns nullptr if no candidate is found.
        // Used by both static-call (FunctionCallNode) and instance-call
        // (MethodCallNode) type inference paths.
        const bytecode::BytecodeProgram::FunctionMetadata* findOverloadMetadata(
            const std::string& callName,
            size_t argCount,
            const std::vector<std::unique_ptr<ast::ASTNode>>& arguments) const;

        // Returns true when the function's declared return type is one of its
        // own generic type parameters (e.g. `<T> deserializeAs(...): T`) and no
        // outer binding can substitute it. Inference must report VOID/empty in
        // that case so the strict assignment validator can't mistake the bare
        // "T" for a concrete class.
        bool isUnboundGenericReturn(
            const bytecode::BytecodeProgram::FunctionMetadata* metadata) const;

        // Helper methods for inferExpressionType
        value::ValueType inferLiteralType(ast::ASTNode* node) const;
        value::ValueType inferVariableType(ast::VariableNode* varNode) const;
        value::ValueType inferFunctionCallType(ast::FunctionCallNode* funcCall) const;
        value::ValueType inferUnaryOperationType(ast::UnaryOpNode* unaryOp) const;
        value::ValueType inferCastType(ast::CastExpression* castExpr) const;
        value::ValueType inferMemberAccessType(ast::MemberAccessNode* memberAccess) const;
        value::ValueType inferMethodCallType(ast::MethodCallNode* methodCall) const;
        value::ValueType inferBinaryOperationType(ast::BinaryOpNode* binOp) const;
        value::ValueType inferIndexAccessType(ast::nodes::expressions::IndexAccessNode* indexAccess) const;

        // Helper methods for inferExpressionClassName
        std::string inferCastClassName(ast::CastExpression* castExpr) const;
        std::string inferVariableClassName(ast::VariableNode* varNode) const;
        std::string inferFunctionCallClassName(ast::FunctionCallNode* funcCall) const;
        std::string inferIndexAccessClassName(ast::nodes::expressions::IndexAccessNode* indexAccess) const;
        std::string inferMemberAccessClassName(ast::MemberAccessNode* memberAccess) const;
        std::string inferMethodCallClassName(ast::MethodCallNode* methodCall) const;

        // Shared operator-overloading detection used by both type and className
        // inference. Sets outLeftClassName to the boxed receiver class when
        // overloading applies; returns false when the expression evaluates as
        // a primitive binary operation.
        bool tryBinaryOperatorOverload(
            ast::BinaryOpNode* binOp,
            value::ValueType leftType,
            value::ValueType rightType,
            std::string& outLeftClassName) const;

        // Per-operator-category helpers for inferBinaryOperationType
        value::ValueType inferOverloadedBinaryType(token::TokenType op) const;
        value::ValueType inferArithmeticBinaryType(
            token::TokenType op,
            value::ValueType leftType,
            value::ValueType rightType) const;
        value::ValueType inferBitwiseBinaryType(
            token::TokenType op,
            value::ValueType leftType,
            value::ValueType rightType) const;

        // Resolve instance-method overload (className::methodName) by argument
        // arity and type compatibility. Mirrors findOverloadMetadata for the
        // method-call path; used by inferMethodCallClassName.
        const bytecode::BytecodeProgram::FunctionMetadata* findInstanceMethodMetadata(
            const std::string& className,
            ast::MethodCallNode* methodCall) const;

        // Per-node-kind className helpers for inferExpressionClassName
        std::string inferArrayCreationClassName(
            ast::nodes::expressions::ArrayCreationNode* arrCreate) const;
        std::string inferArrayLiteralClassName(
            ast::nodes::expressions::ArrayLiteralNode* arrayLit) const;
        std::string inferBinaryOpClassName(ast::BinaryOpNode* binOp) const;
        std::string inferAwaitClassName(
            ast::nodes::expressions::AwaitExpression* awaitExpr) const;

        // Per-node-kind nullable helpers for inferExpressionNullable
        bool inferVariableNullable(ast::VariableNode* varNode) const;
        bool inferMethodCallNullable(ast::MethodCallNode* methodCall) const;
        bool inferMemberAccessNullable(ast::MemberAccessNode* memberAccess) const;
    };
}
