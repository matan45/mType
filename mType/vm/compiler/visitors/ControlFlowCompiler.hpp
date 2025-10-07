#pragma once
#include "CompilerContext.hpp"
#include "../../../ast/nodes/statements/IfNode.hpp"
#include "../../../ast/nodes/statements/WhileNode.hpp"
#include "../../../ast/nodes/statements/DoWhileNode.hpp"
#include "../../../ast/nodes/statements/ForNode.hpp"
#include "../../../ast/nodes/statements/ForEachNode.hpp"
#include "../../../ast/nodes/statements/SwitchNode.hpp"
#include "../../../ast/nodes/statements/CaseNode.hpp"
#include "../../../ast/nodes/statements/DefaultCaseNode.hpp"
#include "../../../ast/nodes/statements/BreakNode.hpp"
#include "../../../ast/nodes/statements/ContinueNode.hpp"
#include "../../../value/ValueType.hpp"

namespace vm::compiler::visitors
{
    /**
     * Compiles control flow nodes to bytecode
     * Handles: if, while, do-while, for, foreach, switch, break, continue
     */
    class ControlFlowCompiler
    {
    public:
        explicit ControlFlowCompiler(CompilerContext& context);
        ~ControlFlowCompiler() = default;

        // Control flow compilation methods
        value::Value compileIf(ast::IfNode* node);
        value::Value compileWhile(ast::WhileNode* node);
        value::Value compileDoWhile(ast::DoWhileNode* node);
        value::Value compileFor(ast::ForNode* node);
        value::Value compileForEach(ast::ForEachNode* node);
        value::Value compileSwitch(ast::SwitchNode* node);
        value::Value compileCase(ast::CaseNode* node);
        value::Value compileDefaultCase(ast::DefaultCaseNode* node);
        value::Value compileBreak(ast::BreakNode* node);
        value::Value compileContinue(ast::ContinueNode* node);

    private:
        CompilerContext& ctx;
    };
}
