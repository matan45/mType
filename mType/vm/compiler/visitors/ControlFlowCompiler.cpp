#include "ControlFlowCompiler.hpp"
#include <cstddef>
#include <cstdint>
#include "StatementCleanup.hpp"
#include "../../bytecode/OpCode.hpp"
#include "../../../errors/ParseException.hpp"
#include "../../../ast/nodes/expressions/BinaryExpNode.hpp"
#include "../../../ast/nodes/expressions/NullNode.hpp"
#include "../../../ast/nodes/expressions/VariableNode.hpp"
#include "../../../ast/nodes/expressions/StringNode.hpp"
#include "../../../token/TokenType.hpp"
#include "../validation/ReturnPathValidator.hpp"
#include "../types/NullConditionFacts.hpp"

namespace vm::compiler::visitors
{
    ControlFlowCompiler::ControlFlowCompiler(CompilerContext& context)
        : ctx(context)
        , tryHelper(std::make_unique<TryStatementHelper>(context))
    {
    }

    value::Value ControlFlowCompiler::compileIf(ast::IfNode* node)
    {
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

        // Jump to else/end if condition is false
        size_t elseJump = ctx.emitter.emitJump(bytecode::OpCode::JUMP_IF_FALSE);

        // Analyze condition for null narrowing (smart casts). Compound
        // short-circuit conditions contribute only facts guaranteed by the
        // selected branch.
        types::NullConditionFacts conditionFacts =
            types::analyzeNullCondition(node->getCondition());

        // Compile then branch with its own scope so its variables don't leak.
        ctx.variableTracker.beginScope();
        {
            types::ScopedNullNarrowing narrowing(ctx.nullNarrowing,
                                                 conditionFacts.whenTrueNonNull);
            // MYT-271: braceless then-bodies (`if (cond) stmt;`) need the same
            // operand-stack cleanup that compileBlock applies after each braced
            // statement. Without this, an assignment body leaks the STORE_LOCAL
            // re-push and a function-call body leaks the return value.
            size_t thenOffsetBefore = ctx.program.getCurrentOffset();
            node->getThenStatement()->accept(ctx.visitor);
            statementCleanup::emitStatementCleanup(ctx, node->getThenStatement(), thenOffsetBefore);
        }
        ctx.variableTracker.endScope();
        ctx.globalRegistry.removeVariablesOutOfScope(ctx.variableTracker.getCurrentScopeDepth());

        if (node->getElseStatement()) {
            // Jump over else branch
            size_t endJump = ctx.emitter.emitJump(bytecode::OpCode::JUMP);

            // Patch else jump
            ctx.emitter.patchJump(elseJump);

            // Compile else branch with its own scope
            ctx.variableTracker.beginScope();
            {
                types::ScopedNullNarrowing narrowing(ctx.nullNarrowing,
                                                     conditionFacts.whenFalseNonNull);
                // MYT-271: braceless else-bodies need the same cleanup as braced.
                size_t elseOffsetBefore = ctx.program.getCurrentOffset();
                node->getElseStatement()->accept(ctx.visitor);
                statementCleanup::emitStatementCleanup(ctx, node->getElseStatement(), elseOffsetBefore);
            }
            ctx.variableTracker.endScope();
            ctx.globalRegistry.removeVariablesOutOfScope(ctx.variableTracker.getCurrentScopeDepth());

            // Patch end jump
            ctx.emitter.patchJump(endJump);
        } else {
            // No else branch, patch jump to end
            ctx.emitter.patchJump(elseJump);
        }

        // Guard-clause narrowing: `if (x == null) { return/throw; }` narrows x
        // to non-null for all subsequent code in the enclosing scope.
        // MYT-381: inside a loop, `continue`/`break` also exit the guarded
        // path; break narrows only when it binds to the loop (not a switch).
        if (!conditionFacts.whenFalseNonNull.empty() && !node->getElseStatement())
        {
            const bool inLoop = ctx.loopManager.isInLoop();
            const bool breakExits = breakBindsToLoop();   // MYT-384: was inLoop && !isInSwitch()
            const bool thenExits = inLoop
                ? validation::ReturnPathValidator::pathAlwaysExitsLoopIteration(
                      node->getThenStatement(), breakExits)
                : validation::ReturnPathValidator::pathAlwaysReturns(node->getThenStatement());
            if (thenExits)
            {
                ctx.nullNarrowing.ensureScope();
                for (const auto& varName : conditionFacts.whenFalseNonNull)
                {
                    ctx.nullNarrowing.narrowToNonNull(varName);
                }
            }
        }

        return std::monostate{};
    }

    value::Value ControlFlowCompiler::compileSwitch(ast::SwitchNode* node)
    {
        // Capture the entry offset before the switch expression so compileBreak
        // can resolve break targets by nesting order (MYT-382). This is the
        // smallest possible offset for the switch, which is strictly safe.
        ctx.switchManager.enterSwitch(ctx.program.getCurrentOffset());

        // Compile switch expression
        node->getExpression()->accept(ctx.visitor);

        const auto& cases = node->getCases();

        bool canUseStringSwitch = true;
        size_t stringCaseCount = 0;
        for (const auto& caseNode : cases)
        {
            if (dynamic_cast<ast::DefaultCaseNode*>(caseNode.get()))
            {
                continue;
            }
            auto* regularCase = dynamic_cast<ast::CaseNode*>(caseNode.get());
            auto* stringValue = regularCase
                ? dynamic_cast<ast::StringNode*>(regularCase->getValue())
                : nullptr;
            if (!stringValue)
            {
                canUseStringSwitch = false;
                break;
            }
            ++stringCaseCount;
        }

        if (canUseStringSwitch && stringCaseCount > 0)
        {
            std::vector<size_t> targetOperandByCase(cases.size(), SIZE_MAX);
            size_t defaultCaseIndex = SIZE_MAX;

            std::vector<uint64_t> operands;
            operands.reserve(2 + stringCaseCount * 2);
            operands.push_back(0); // default/end target, patched after bodies.
            operands.push_back(static_cast<uint64_t>(stringCaseCount));

            size_t stringOrdinal = 0;
            for (size_t i = 0; i < cases.size(); ++i)
            {
                if (dynamic_cast<ast::DefaultCaseNode*>(cases[i].get()))
                {
                    defaultCaseIndex = i;
                    continue;
                }

                auto* regularCase = dynamic_cast<ast::CaseNode*>(cases[i].get());
                auto* stringValue = dynamic_cast<ast::StringNode*>(regularCase->getValue());
                const size_t stringIndex =
                    ctx.program.getConstantPool().addString(stringValue->getValue());
                operands.push_back(static_cast<uint64_t>(stringIndex));
                targetOperandByCase[i] = 3 + stringOrdinal * 2;
                operands.push_back(0); // case body target, patched below.
                ++stringOrdinal;
            }

            const size_t switchOffset = ctx.program.getCurrentOffset();
            ctx.emitter.emitWithLocation(bytecode::OpCode::SWITCH_STRING, operands, node);

            for (size_t i = 0; i < cases.size(); ++i)
            {
                const size_t bodyStart = ctx.program.getCurrentOffset();
                if (i == defaultCaseIndex)
                {
                    auto& switchInstr = ctx.program.getMutableInstruction(switchOffset);
                    switchInstr.setOperandAt(0, static_cast<uint64_t>(bodyStart));
                }
                else if (targetOperandByCase[i] != SIZE_MAX)
                {
                    auto& switchInstr = ctx.program.getMutableInstruction(switchOffset);
                    switchInstr.setOperandAt(targetOperandByCase[i],
                                             static_cast<uint64_t>(bodyStart));
                }

                compileCaseBody(cases[i].get());
            }

            const size_t endOffset = ctx.program.getCurrentOffset();
            if (defaultCaseIndex == SIZE_MAX)
            {
                auto& switchInstr = ctx.program.getMutableInstruction(switchOffset);
                switchInstr.setOperandAt(0, static_cast<uint64_t>(endOffset));
            }

            for (size_t breakJump : ctx.switchManager.getBreakJumps()) {
                ctx.emitter.patchJump(breakJump);
            }

            ctx.switchManager.exitSwitch();
            return std::monostate{};
        }

        std::vector<size_t> caseBodyStarts;
        size_t defaultBodyStart = SIZE_MAX;
        size_t defaultCaseIndex = SIZE_MAX;

        ctx.variableTracker.beginScope();
        ctx.variableTracker.declareLocal("$switch_val", value::ValueType::VOID);
        ctx.functionFrameManager.updateMaxLocalSlot(ctx.variableTracker.getNextLocalSlot());
        size_t switchValueSlot = ctx.variableTracker.getNextLocalSlot() - 1;
        if (ctx.functionFrameManager.isInFunction()) {
            switchValueSlot -= ctx.functionFrameManager.currentFrame().localStartSlot;
        }
        ctx.emitter.emitWithLocation(bytecode::OpCode::STORE_LOCAL,
                                     static_cast<uint64_t>(switchValueSlot), node);
        ctx.emitter.emitWithLocation(bytecode::OpCode::POP, node);

        // First pass: generate comparisons
        for (size_t i = 0; i < cases.size(); ++i) {
            if (dynamic_cast<ast::DefaultCaseNode*>(cases[i].get())) {
                defaultCaseIndex = i;
                caseBodyStarts.push_back(SIZE_MAX);
            } else if (auto* regularCase = dynamic_cast<ast::CaseNode*>(cases[i].get())) {
                ctx.emitter.emitWithLocation(bytecode::OpCode::LOAD_LOCAL,
                                             static_cast<uint64_t>(switchValueSlot), regularCase);
                regularCase->getValue()->accept(ctx.visitor);
                ctx.emitter.emitWithLocation(bytecode::OpCode::EQ, regularCase);
                size_t jumpToCaseBody = ctx.emitter.emitJump(bytecode::OpCode::JUMP_IF_TRUE);
                caseBodyStarts.push_back(jumpToCaseBody);
            }
        }

        size_t jumpToDefaultOrEnd = ctx.emitter.emitJump(bytecode::OpCode::JUMP);

        // Second pass: compile case bodies
        for (size_t i = 0; i < cases.size(); ++i) {
            if (caseBodyStarts[i] != SIZE_MAX) {
                ctx.emitter.patchJump(caseBodyStarts[i]);
            }

            if (i == defaultCaseIndex) {
                defaultBodyStart = ctx.program.getCurrentOffset();
            }

            // Compile case statements
            compileCaseBody(cases[i].get());
        }

        size_t implicitEndJump = ctx.emitter.emitJump(bytecode::OpCode::JUMP);

        // Patch jump to default or end
        ctx.emitter.patchJump(jumpToDefaultOrEnd);
        if (defaultBodyStart != SIZE_MAX) {
            ctx.emitter.emitWithLocation(bytecode::OpCode::JUMP, static_cast<uint64_t>(defaultBodyStart), node);
        }

        // Patch all break jumps
        ctx.emitter.patchJump(implicitEndJump);
        for (size_t breakJump : ctx.switchManager.getBreakJumps()) {
            ctx.emitter.patchJump(breakJump);
        }

        ctx.switchManager.exitSwitch();
        ctx.variableTracker.endScope();

        return std::monostate{};
    }

    void ControlFlowCompiler::compileCaseBody(ast::ASTNode* caseNode)
    {
        const std::vector<std::unique_ptr<ast::ASTNode>>* statements = nullptr;
        if (auto* defaultCase = dynamic_cast<ast::DefaultCaseNode*>(caseNode)) {
            statements = &defaultCase->getStatements();
        } else if (auto* regularCase = dynamic_cast<ast::CaseNode*>(caseNode)) {
            statements = &regularCase->getStatements();
        }
        if (!statements) {
            return;
        }

        for (const auto& stmt : *statements) {
            size_t offsetBefore = ctx.program.getCurrentOffset();
            stmt->accept(ctx.visitor);
            statementCleanup::emitStatementCleanup(ctx, stmt.get(), offsetBefore);
        }
    }

    value::Value ControlFlowCompiler::compileCase(ast::CaseNode* node)
    {
        // Case nodes are handled by SwitchNode
        return std::monostate{};
    }

    value::Value ControlFlowCompiler::compileDefaultCase(ast::DefaultCaseNode* node)
    {
        // Default case nodes are handled by SwitchNode
        return std::monostate{};
    }

    value::Value ControlFlowCompiler::compileMatch(ast::MatchNode* node)
    {
        using namespace ast::nodes::statements;

        // 1. Compile match expression and store in a temp local
        node->getExpression()->accept(ctx.visitor);

        ctx.variableTracker.beginScope();
        ctx.variableTracker.declareLocal("$match_val", value::ValueType::VOID);
        ctx.functionFrameManager.updateMaxLocalSlot(ctx.variableTracker.getNextLocalSlot());
        size_t matchSlot = ctx.variableTracker.getNextLocalSlot() - 1;
        size_t relativeMatchSlot = matchSlot;
        if (ctx.functionFrameManager.isInFunction()) {
            relativeMatchSlot = matchSlot - ctx.functionFrameManager.currentFrame().localStartSlot;
        }
        ctx.emitter.emitWithLocation(bytecode::OpCode::STORE_LOCAL,
            static_cast<uint64_t>(relativeMatchSlot), node);

        const auto& cases = node->getCases();
        std::vector<size_t> endJumps;

        for (size_t i = 0; i < cases.size(); ++i) {
            if (auto* matchCase = dynamic_cast<MatchCaseNode*>(cases[i].get())) {
                PatternKind kind = matchCase->getPatternKind();

                if (kind == PatternKind::TYPE) {
                    // TYPE PATTERN: case TypeName varName ->
                    ctx.emitter.emitWithLocation(bytecode::OpCode::LOAD_LOCAL,
                        static_cast<uint64_t>(relativeMatchSlot), matchCase);

                    size_t typeNameIndex = ctx.program.getConstantPool().addString(matchCase->getTypeName());
                    ctx.emitter.emitWithLocation(bytecode::OpCode::INSTANCEOF,
                        static_cast<uint64_t>(typeNameIndex), matchCase);

                    size_t jumpToNext = ctx.emitter.emitJump(bytecode::OpCode::JUMP_IF_FALSE);

                    // Bind the variable in a new scope
                    ctx.variableTracker.beginScope();
                    ctx.variableTracker.declareLocal(
                        matchCase->getBindingName(),
                        value::ValueType::OBJECT,
                        matchCase->getTypeName()
                    );
                    ctx.functionFrameManager.updateMaxLocalSlot(ctx.variableTracker.getNextLocalSlot());
                    size_t bindSlot = ctx.variableTracker.getNextLocalSlot() - 1;
                    size_t relBindSlot = bindSlot;
                    if (ctx.functionFrameManager.isInFunction()) {
                        relBindSlot = bindSlot - ctx.functionFrameManager.currentFrame().localStartSlot;
                    }

                    // Load match value and store into binding variable
                    ctx.emitter.emitWithLocation(bytecode::OpCode::LOAD_LOCAL,
                        static_cast<uint64_t>(relativeMatchSlot), matchCase);
                    ctx.emitter.emitWithLocation(bytecode::OpCode::STORE_LOCAL,
                        static_cast<uint64_t>(relBindSlot), matchCase);

                    // Compile body
                    matchCase->getBody()->accept(ctx.visitor);

                    ctx.variableTracker.endScope();

                    endJumps.push_back(ctx.emitter.emitJump(bytecode::OpCode::JUMP));
                    ctx.emitter.patchJump(jumpToNext);
                }
                else if (kind == PatternKind::VALUE) {
                    // VALUE PATTERN: case expr ->
                    ctx.emitter.emitWithLocation(bytecode::OpCode::LOAD_LOCAL,
                        static_cast<uint64_t>(relativeMatchSlot), matchCase);

                    matchCase->getValueExpression()->accept(ctx.visitor);

                    ctx.emitter.emitWithLocation(bytecode::OpCode::EQ, matchCase);

                    size_t jumpToNext = ctx.emitter.emitJump(bytecode::OpCode::JUMP_IF_FALSE);

                    matchCase->getBody()->accept(ctx.visitor);

                    endJumps.push_back(ctx.emitter.emitJump(bytecode::OpCode::JUMP));
                    ctx.emitter.patchJump(jumpToNext);
                }
                else if (kind == PatternKind::NULL_PATTERN) {
                    // NULL PATTERN: case null ->
                    ctx.emitter.emitWithLocation(bytecode::OpCode::LOAD_LOCAL,
                        static_cast<uint64_t>(relativeMatchSlot), matchCase);

                    ctx.emitter.emitWithLocation(bytecode::OpCode::PUSH_NULL, matchCase);
                    ctx.emitter.emitWithLocation(bytecode::OpCode::EQ, matchCase);

                    size_t jumpToNext = ctx.emitter.emitJump(bytecode::OpCode::JUMP_IF_FALSE);

                    matchCase->getBody()->accept(ctx.visitor);

                    endJumps.push_back(ctx.emitter.emitJump(bytecode::OpCode::JUMP));
                    ctx.emitter.patchJump(jumpToNext);
                }
            }
            else if (auto* defaultCase = dynamic_cast<MatchDefaultNode*>(cases[i].get())) {
                // DEFAULT: default ->
                defaultCase->getBody()->accept(ctx.visitor);
                endJumps.push_back(ctx.emitter.emitJump(bytecode::OpCode::JUMP));
            }
        }

        // Patch all end jumps
        for (size_t jump : endJumps) {
            ctx.emitter.patchJump(jump);
        }

        ctx.variableTracker.endScope();

        return std::monostate{};
    }

    bool ControlFlowCompiler::breakBindsToLoop() const
    {
        if (!ctx.loopManager.isInLoop()) {
            return false;
        }
        if (!ctx.switchManager.isInSwitch()) {
            return true;
        }
        // Both active: the loop is nested deeper iff it was entered later
        // (strictly larger offset). Equal offsets cannot occur for genuine
        // nesting — compileSwitch emits >= 1 instruction before any case body.
        return ctx.loopManager.getLoopStart()
             > ctx.switchManager.getCurrentSwitchEntryOffset();
    }

    value::Value ControlFlowCompiler::compileBreak(ast::BreakNode* node)
    {
        // Synthesize one STACK_SCOPE_LEAVE per active stack-scope between
        // the current emission point and the enclosing loop's entry depth
        // so pool slots allocated inside the loop body are returned before
        // the break jump. Switch contexts are not stack-scope-emitting so
        // only the loop branch needs the delta.
        //
        // MYT-382/MYT-384: bind break to the INNERMOST breakable construct.
        // The innermost-construct decision lives in breakBindsToLoop() so this
        // emit path and compileIf's guard-narrowing predicate can't drift.
        if (!ctx.switchManager.isInSwitch() && !ctx.loopManager.isInLoop()) {
            throw errors::ParseException("Break outside of loop or switch");
        }
        const bool breakToSwitch = !breakBindsToLoop();

        if (breakToSwitch) {
            size_t breakJump = ctx.emitter.emitJump(bytecode::OpCode::JUMP);
            ctx.switchManager.registerBreak(breakJump);
        } else {
            const uint32_t leaveCount = ctx.loopManager.getOpenStackScopeDepthInLoop();
            for (uint32_t i = 0; i < leaveCount; ++i) {
                ctx.emitter.emitWithLocation(bytecode::OpCode::STACK_SCOPE_LEAVE, node);
            }
            // If we're in a try block with a finally (but NOT already inside
            // the finally), register the break jump with the exception manager
            // so the finally block creates a trampoline that executes finally
            // before breaking the loop.
            if (ctx.exceptionManager.hasPendingFinally() && !ctx.exceptionManager.isInFinally()) {
                size_t breakJump = ctx.emitter.emitJump(bytecode::OpCode::JUMP);
                ctx.exceptionManager.registerBreakJump(breakJump);
            } else {
                size_t breakJump = ctx.emitter.emitJump(bytecode::OpCode::JUMP);
                ctx.loopManager.registerBreak(breakJump);
            }
        }
        return std::monostate{};
    }

    value::Value ControlFlowCompiler::compileContinue(ast::ContinueNode* node)
    {
        if (!ctx.loopManager.isInLoop()) {
            throw errors::ParseException("Continue outside of loop");
        }

        // Same stack-scope unwind as compileBreak: emit one LEAVE per active
        // scope inside the current loop body so re-entering the body via the
        // continue target leaves stackObjectsCount at the loop-entry value.
        const uint32_t leaveCount = ctx.loopManager.getOpenStackScopeDepthInLoop();
        for (uint32_t i = 0; i < leaveCount; ++i) {
            ctx.emitter.emitWithLocation(bytecode::OpCode::STACK_SCOPE_LEAVE, node);
        }

        // If we're in a try block with a finally (but NOT already inside the
        // finally), register the continue jump with the exception manager so
        // the finally block creates a trampoline that executes finally before
        // continuing the loop.
        if (ctx.exceptionManager.hasPendingFinally() && !ctx.exceptionManager.isInFinally()) {
            size_t continueJump = ctx.emitter.emitJump(bytecode::OpCode::JUMP);
            ctx.exceptionManager.registerContinueJump(continueJump);
        } else {
            size_t continueJump = ctx.emitter.emitJump(bytecode::OpCode::JUMP);
            ctx.loopManager.registerContinue(continueJump);
        }
        return std::monostate{};
    }

    value::Value ControlFlowCompiler::compileTry(ast::TryNode* node)
    {
        return tryHelper->compileTry(node);
    }

    value::Value ControlFlowCompiler::compileThrow(ast::ThrowNode* node)
    {
        // Compile the exception expression (should evaluate to an object)
        node->getException()->accept(ctx.visitor);

        // Emit THROW instruction with source location
        ctx.emitter.emitWithLocation(bytecode::OpCode::THROW, node);

        return std::monostate{};
    }
}
