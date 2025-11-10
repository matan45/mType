#pragma once
#include "../../../ast/ASTNode.hpp"
#include "../../../value/ValueType.hpp"
#include "../../bytecode/BytecodeProgram.hpp"
#include "../../../environment/Environment.hpp"
#include "../variables/VariableTracker.hpp"
#include "../variables/GlobalVariableRegistry.hpp"
#include <string>
#include <memory>
#include <unordered_map>
#include <vector>

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

        // Set generic type bindings stack (reference to context's stack)
        void setGenericTypeBindingsStack(const std::vector<std::unordered_map<std::string, std::string>>* stack);

        // PHASE 3: Set reference to resolved function call types cache
        void setResolvedFunctionCallTypes(const std::unordered_map<const ast::ASTNode*, std::string>* cache);

    private:
        const bytecode::BytecodeProgram& program;
        std::shared_ptr<environment::Environment> environment;
        const variables::VariableTracker& variableTracker;
        const variables::GlobalVariableRegistry& globalRegistry;
        const std::vector<std::unordered_map<std::string, std::string>>* genericTypeBindingsStack = nullptr;
        const std::unordered_map<const ast::ASTNode*, std::string>* resolvedFunctionCallTypes = nullptr;  // PHASE 3

        // Helper to resolve generic types
        std::string resolveGenericType(const std::string& typeName) const;

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
    };
}
