#pragma once
#include "CompilerContext.hpp"
#include <cstddef>
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

        // MYT-352: emit STACK_SCOPE_LEAVE × currentStackScopeDepth so a return
        // drains open stack-object scopes before transferring control. Required
        // for JIT inlining to be safe — when the callee frame is elided, the
        // structural LEAVE after the body never runs, so the return path itself
        // must carry the cleanup (mirrors break/continue's existing precedent
        // in ControlFlowCompiler). Safe by the escape-analysis invariant: any
        // object in the frame's stackObjects[] at return time has no live
        // references past this scope.
        void emitStackScopeLeavesBeforeReturn(ast::ASTNode* node);

        // Phase 4: Auto-boxing helper for return statements
        bool tryEmitReturnAutoBoxing(ast::ASTNode* returnValue);

        // Helper methods for compileLambda
        std::vector<variables::VariableTracker::LocalVariable> captureScopeVariables();
        std::vector<std::string> setupLambdaFrame(ast::LambdaNode* node,
                             const std::vector<variables::VariableTracker::LocalVariable>& capturedVars,
                             const std::string& lambdaFuncName);
        void emitLambdaInstruction(size_t lambdaStart, ast::LambdaNode* node,
                                   const std::vector<variables::VariableTracker::LocalVariable>& capturedVars,
                                   size_t currentFrameStart, const std::vector<variables::VariableTracker::LocalVariable>& currentLocals,
                                   const std::string& lambdaFuncName);

        // MYT-280: returns true iff `name` is a generic type parameter currently
        // in scope (free function, method, or enclosing class) AND that parameter
        // MAY be instantiated with a nullable type — i.e. it has no constraint
        // or any of its constraints is nullable. Returns false when the name is
        // not a generic parameter in scope (caller falls through to normal
        // validation) or when every constraint is non-nullable.
        bool isGenericParamWithPossiblyNullableInstantiation(const std::string& name) const;
    };
}
