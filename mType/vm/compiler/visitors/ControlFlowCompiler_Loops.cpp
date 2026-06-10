#include "ControlFlowCompiler.hpp"
#include <cstddef>
#include <cstdint>
#include "StatementCleanup.hpp"
#include "../../bytecode/OpCode.hpp"
#include "../../../ast/nodes/expressions/BinaryExpNode.hpp"
#include "../../../ast/nodes/expressions/NullNode.hpp"
#include "../../../ast/nodes/expressions/VariableNode.hpp"
#include "../../../token/TokenType.hpp"
#include "../types/NullConditionFacts.hpp"

namespace vm::compiler::visitors
{
    value::Value ControlFlowCompiler::compileWhile(ast::WhileNode* node)
    {
        // Emit LOOP_START marker for optimization passes
        ctx.emitter.emitWithLocation(bytecode::OpCode::LOOP_START, node);

        size_t loopStart = ctx.program.getCurrentOffset();

        // Compile condition
        node->getCondition()->accept(ctx.visitor);

        // Auto-unbox Bool objects to primitive bool
        value::ValueType conditionType = ctx.typeInference.inferExpressionType(node->getCondition());
        if (conditionType == value::ValueType::OBJECT)
        {
            std::string conditionClassName = ctx.typeInference.inferExpressionClassName(node->getCondition());
            if (conditionClassName == "Bool" && ctx.env->findClass("Bool"))
            {
                size_t methodNameIndex = ctx.program.getConstantPool().addString("getValue");
                ctx.emitter.emitWithLocation(bytecode::OpCode::CALL_METHOD,
                                             static_cast<uint64_t>(methodNameIndex),
                                             0u,
                                             node->getCondition());
            }
        }

        // Jump to end if condition is false
        size_t exitJump = ctx.emitter.emitJump(bytecode::OpCode::JUMP_IF_FALSE);

        ctx.loopManager.enterLoop(loopStart);

        types::NullConditionFacts conditionFacts =
            types::analyzeNullCondition(node->getCondition());

        // Compile body with its own scope
        ctx.variableTracker.beginScope();
        {
            types::ScopedNullNarrowing narrowing(ctx.nullNarrowing,
                                                 conditionFacts.whenTrueNonNull,
                                                 /*forceScope=*/true);
            // MYT-271: braceless while-bodies need the same cleanup as braced.
            size_t whileBodyOffsetBefore = ctx.program.getCurrentOffset();
            node->getBody()->accept(ctx.visitor);
            statementCleanup::emitStatementCleanup(ctx, node->getBody(), whileBodyOffsetBefore);
        }
        ctx.variableTracker.endScope();
        ctx.globalRegistry.removeVariablesOutOfScope(ctx.variableTracker.getCurrentScopeDepth());

        // Jump back to start
        ctx.emitter.emitLoop(loopStart);

        ctx.emitter.emitWithLocation(bytecode::OpCode::LOOP_END, node);

        // Exit loop and patch jumps
        ctx.emitter.patchJump(exitJump);
        for (size_t breakJump : ctx.loopManager.getBreakJumps()) {
            ctx.emitter.patchJump(breakJump);
        }
        for (size_t continueJump : ctx.loopManager.getContinueJumps()) {
            ctx.program.patchJump(continueJump, static_cast<uint64_t>(loopStart));
        }
        ctx.loopManager.exitLoop();

        return std::monostate{};
    }

    value::Value ControlFlowCompiler::compileDoWhile(ast::DoWhileNode* node)
    {
        ctx.emitter.emitWithLocation(bytecode::OpCode::LOOP_START, node);

        size_t loopStart = ctx.program.getCurrentOffset();

        ctx.loopManager.enterLoop(loopStart);

        // Compile body with its own scope
        ctx.variableTracker.beginScope();
        {
            // MYT-381: bound guard-clause narrowing to the body (no condition
            // facts apply — a do-while body runs before the condition).
            types::ScopedNullNarrowing narrowing(ctx.nullNarrowing,
                                                 std::vector<std::string>{},
                                                 /*forceScope=*/true);
            // MYT-271: braceless do-while bodies need the same cleanup as braced.
            size_t doBodyOffsetBefore = ctx.program.getCurrentOffset();
            node->getBody()->accept(ctx.visitor);
            statementCleanup::emitStatementCleanup(ctx, node->getBody(), doBodyOffsetBefore);
        }
        ctx.variableTracker.endScope();
        ctx.globalRegistry.removeVariablesOutOfScope(ctx.variableTracker.getCurrentScopeDepth());

        // MYT-383: continue in a do-while must jump to the condition, not the
        // body start (loopStart). The condition is emitted after the body with
        // no separate label, so record its offset here for the continue patch.
        size_t conditionStart = ctx.program.getCurrentOffset();

        // Compile condition
        node->getCondition()->accept(ctx.visitor);

        // Auto-unbox Bool objects to primitive bool
        value::ValueType conditionType = ctx.typeInference.inferExpressionType(node->getCondition());
        if (conditionType == value::ValueType::OBJECT)
        {
            std::string conditionClassName = ctx.typeInference.inferExpressionClassName(node->getCondition());
            if (conditionClassName == "Bool" && ctx.env->findClass("Bool"))
            {
                size_t methodNameIndex = ctx.program.getConstantPool().addString("getValue");
                ctx.emitter.emitWithLocation(bytecode::OpCode::CALL_METHOD,
                                             static_cast<uint64_t>(methodNameIndex),
                                             0u,
                                             node->getCondition());
            }
        }

        // Jump back to start if condition is true
        size_t continueJump = ctx.emitter.emitJump(bytecode::OpCode::JUMP_IF_TRUE);
        ctx.program.patchJump(continueJump, static_cast<uint64_t>(loopStart));

        ctx.emitter.emitWithLocation(bytecode::OpCode::LOOP_END, node);

        // Exit loop and patch jumps
        for (size_t breakJump : ctx.loopManager.getBreakJumps()) {
            ctx.emitter.patchJump(breakJump);
        }
        for (size_t contJump : ctx.loopManager.getContinueJumps()) {
            ctx.program.patchJump(contJump, static_cast<uint64_t>(conditionStart));
        }
        ctx.loopManager.exitLoop();

        return std::monostate{};
    }

    value::Value ControlFlowCompiler::compileFor(ast::ForNode* node)
    {
        ctx.variableTracker.beginScope();

        // Compile initializer. STORE_LOCAL / STORE_VAR / SET_FIELD re-push the
        // stored value at runtime (for expression-context cascades like
        // `int x = (a = 5)`), so the init site needs a POP in statement
        // context. MYT-271: also covers Path A (function-call initializer)
        // via the shared helper — mirrors the update-path POP below.
        if (node->getInitialization()) {
            size_t offsetBefore = ctx.program.getCurrentOffset();
            node->getInitialization()->accept(ctx.visitor);
            statementCleanup::emitStatementCleanup(ctx, node->getInitialization(), offsetBefore);
        }

        ctx.emitter.emitWithLocation(bytecode::OpCode::LOOP_START, node);

        size_t loopStart = ctx.program.getCurrentOffset();

        // Compile condition
        types::NullConditionFacts forConditionFacts;
        if (node->getCondition()) {
            forConditionFacts = types::analyzeNullCondition(node->getCondition());
            node->getCondition()->accept(ctx.visitor);
        } else {
            // No condition means infinite loop
            ctx.emitter.emitWithLocation(bytecode::OpCode::PUSH_BOOL, 1, node);
        }

        size_t exitJump = ctx.emitter.emitJump(bytecode::OpCode::JUMP_IF_FALSE);

        // Jump over increment to body
        size_t bodyJump = ctx.emitter.emitJump(bytecode::OpCode::JUMP);

        // Increment location (continue jumps here)
        size_t incrementStart = ctx.program.getCurrentOffset();
        if (node->getUpdate()) {
            node->getUpdate()->accept(ctx.visitor);
            ctx.emitter.emitWithLocation(bytecode::OpCode::POP, node);  // Discard increment result
        }

        // Jump back to condition
        ctx.emitter.emitLoop(loopStart);

        // Body starts here
        ctx.emitter.patchJump(bodyJump);

        ctx.loopManager.enterLoop(loopStart, incrementStart);

        // Compile body
        // MYT-271: braceless for-bodies need the same cleanup as braced.
        if (node->getBody()) {
            types::ScopedNullNarrowing narrowing(ctx.nullNarrowing,
                                                 forConditionFacts.whenTrueNonNull,
                                                 /*forceScope=*/true);
            size_t forBodyOffsetBefore = ctx.program.getCurrentOffset();
            node->getBody()->accept(ctx.visitor);
            statementCleanup::emitStatementCleanup(ctx, node->getBody(), forBodyOffsetBefore);
        }

        // Jump to increment
        ctx.emitter.emitLoop(incrementStart);

        ctx.emitter.emitWithLocation(bytecode::OpCode::LOOP_END, node);

        // Exit loop and patch jumps
        ctx.emitter.patchJump(exitJump);
        for (size_t breakJump : ctx.loopManager.getBreakJumps()) {
            ctx.emitter.patchJump(breakJump);
        }
        for (size_t continueJump : ctx.loopManager.getContinueJumps()) {
            ctx.program.patchJump(continueJump, static_cast<uint64_t>(incrementStart));
        }
        ctx.loopManager.exitLoop();

        ctx.variableTracker.endScope();
        ctx.globalRegistry.removeVariablesOutOfScope(ctx.variableTracker.getCurrentScopeDepth());

        return std::monostate{};
    }
}
