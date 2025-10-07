#pragma once
#include "CompilerContext.hpp"
#include "../../../ast/nodes/functions/FunctionNode.hpp"
#include "../../../ast/nodes/functions/FunctionCallNode.hpp"
#include "../../../ast/nodes/functions/ReturnNode.hpp"
#include "../../../ast/nodes/statements/NativeFunctionNode.hpp"
#include "../../../ast/nodes/expressions/LambdaNode.hpp"
#include "../../../value/ValueType.hpp"

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
        value::Value compileNativeFunction(ast::NativeFunctionNode* node);
        value::Value compileLambda(ast::LambdaNode* node);

    private:
        CompilerContext& ctx;
    };
}
