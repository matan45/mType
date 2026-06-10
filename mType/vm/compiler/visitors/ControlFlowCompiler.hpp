#pragma once
#include "CompilerContext.hpp"
#include "TryStatementHelper.hpp"
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
#include "../../../ast/nodes/statements/TryNode.hpp"
#include "../../../ast/nodes/statements/CatchNode.hpp"
#include "../../../ast/nodes/statements/ThrowNode.hpp"
#include "../../../ast/nodes/statements/MatchNode.hpp"
#include "../../../ast/nodes/statements/MatchCaseNode.hpp"
#include "../../../ast/nodes/statements/MatchDefaultNode.hpp"
#include "../../../value/ValueType.hpp"
#include <memory>

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
        value::Value compileMatch(ast::MatchNode* node);
        value::Value compileBreak(ast::BreakNode* node);
        value::Value compileContinue(ast::ContinueNode* node);

        // Exception handling compilation methods
        value::Value compileTry(ast::TryNode* node);
        value::Value compileThrow(ast::ThrowNode* node);

    private:
        // Compile the statement body of a case/default node, emitting per-statement
        // cleanup after each. Shared by the string- and int-switch lowering so the
        // cleanup wiring lives in one place (a missed cleanup on one branch was the
        // exact defect class this consolidation guards against).
        void compileCaseBody(ast::ASTNode* caseNode);

        // MYT-382/MYT-384: a `break` at the current compile point binds to the
        // INNERMOST breakable construct. Returns true iff such a break exits the
        // innermost LOOP iteration (binds to the loop, not an enclosing switch).
        // Single source of truth shared by compileBreak (emit) and compileIf
        // (guard-narrowing predicate) so the two can't drift.
        bool breakBindsToLoop() const;

        CompilerContext& ctx;
        std::unique_ptr<TryStatementHelper> tryHelper;
    };
}
