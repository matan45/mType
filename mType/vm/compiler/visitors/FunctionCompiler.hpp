#pragma once
#include "CompilerContext.hpp"
#include "FunctionCallHelper.hpp"
#include "../../../ast/nodes/functions/FunctionNode.hpp"

#include "../../../value/ValueType.hpp"
#include <memory>

namespace vm::compiler::visitors
{
    /**
     * Compiles function-related nodes to bytecode
     * Handles: functions, function calls, returns, native functions, lambdas
     */
    class FunctionCompiler
    {
    public:
        explicit FunctionCompiler(CompilerContext& context);
        ~FunctionCompiler() = default;

        // Function compilation methods
        value::Value compileFunction(ast::FunctionNode* node);
        value::Value compileFunctionCall(ast::FunctionCallNode* node);
        value::Value compileReturn(ast::ReturnNode* node);
        value::Value compileLambda(ast::LambdaNode* node);

    private:
        CompilerContext& ctx;
        std::unique_ptr<FunctionCallHelper> callHelper;

        // Helper methods for compileReturn
        void validateReturnType(ast::ReturnNode* node, ast::ASTNode* returnValue);
        void emitReturnWithFinally(ast::ReturnNode* node, ast::ASTNode* returnValue);
        void emitReturnWithOuterFinally(ast::ReturnNode* node, ast::ASTNode* returnValue);
        void emitReturnValueBytecode(ast::ReturnNode* node, ast::ASTNode* returnValue);

        // Helper methods for compileLambda
        std::vector<variables::VariableTracker::LocalVariable> captureScopeVariables();
        void setupLambdaFrame(ast::LambdaNode* node,
                             const std::vector<variables::VariableTracker::LocalVariable>& capturedVars);
        void emitLambdaInstruction(size_t lambdaStart, ast::LambdaNode* node,
                                   const std::vector<variables::VariableTracker::LocalVariable>& capturedVars,
                                   size_t currentFrameStart, const std::vector<variables::VariableTracker::LocalVariable>& currentLocals,
                                   const std::string& lambdaFuncName);

        // Type validation helper
        bool isValidTypeName(const std::string& typeName, const std::vector<std::string>& validGenericParams);
    };
}
