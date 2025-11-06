#pragma once
#include "CompilerContext.hpp"
#include "../../../ast/nodes/functions/FunctionCallNode.hpp"
#include "../../../value/ValueType.hpp"
#include <vector>
#include <string>
#include <memory>

namespace vm::compiler::visitors
{
    /**
     * Helper class for compiling function calls
     * Extracted from FunctionCompiler to improve Single Responsibility Principle compliance
     * Handles: function call compilation, parameter validation, generic type binding, method calls
     */
    class FunctionCallHelper
    {
    public:
        explicit FunctionCallHelper(CompilerContext& context);
        ~FunctionCallHelper() = default;

        // Main compilation method
        value::Value compileFunctionCall(ast::FunctionCallNode* node);

    private:
        CompilerContext& ctx;

        // Helper methods
        bool setupGenericTypeBindings(ast::FunctionCallNode* node, const std::string& functionName);

        std::string inferTypeFromArgument(ast::ASTNode* argument);

        void validateFunctionParameters(ast::FunctionCallNode* node, const std::string& functionName,
                                       const std::vector<std::unique_ptr<ast::ASTNode>>& arguments);

        void emitStaticMethodCall(ast::FunctionCallNode* node, const std::string& functionName,
                                 const std::vector<std::unique_ptr<ast::ASTNode>>& arguments);

        void emitMethodCallInClassContext(ast::FunctionCallNode* node, const std::string& functionName,
                                         const std::vector<std::unique_ptr<ast::ASTNode>>& arguments);

        void emitRegularFunctionCall(ast::FunctionCallNode* node, const std::string& functionName,
                                    const std::vector<std::unique_ptr<ast::ASTNode>>& arguments);

        // Phase 4: Auto-boxing helper
        void compileArgumentWithAutoBoxing(ast::ASTNode* argument,
                                          const std::string& expectedTypeName,
                                          value::ValueType expectedType);
    };
}
