#include "BytecodeRuntimeOptimizationTests.hpp"

#include "BytecodeOptimizationTestSuite.hpp"
#include "JitFrameAnalysisTests.hpp"
#include "../../ast/nodes/statements/ProgramNode.hpp"
#include "../../services/BytecodeExecutionStrategy.hpp"
#include "../../services/ImportResolver.hpp"
#include "../../services/ScriptAPI.hpp"
#include "../../vm/bytecode/BytecodeProgram.hpp"
#include "../../vm/bytecode/OpCode.hpp"
#include "../../vm/compiler/BytecodeCompiler.hpp"
#include "../../vm/jit/JitCodeCache.hpp"
#include "../../vm/jit/JitProfiler.hpp"
#include "../../vm/jit/LoopProfiler.hpp"
#include "../../vm/jit/OSRManager.hpp"
#include "../../vm/jit/ic/InlineCacheTable.hpp"
#include "../../vm/optimization/InlineAnalysis.hpp"
#include "../../vm/runtime/VirtualMachine.hpp"
#include "../../environment/registry/ClassDefinition.hpp"

#include <asmjit/x86.h>
#include <array>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace tests::testSuite
{
    using services::ScriptAPI;
    using vm::bytecode::OpCode;

    namespace
    {
        void require(bool condition, const std::string& message)
        {
            if (!condition) throw std::runtime_error(message);
        }

        void emitPushInt(
            vm::bytecode::BytecodeProgram& program, int64_t value)
        {
            const size_t index = program.getConstantPool().addInteger(value);
            program.emit(OpCode::PUSH_INT, static_cast<uint64_t>(index));
        }
    }

    void registerBytecodeRuntimeOptimizationTests(
        BytecodeOptimizationTestSuite& suite)
    {
        suite.addCallbackTest(
            "Instruction tagged overflow preserves logical operands", "",
            [](ScriptAPI&) {
                using Instruction = vm::bytecode::BytecodeProgram::Instruction;
                require(sizeof(Instruction) == 32,
                    "in-memory instructions must remain two per 64-byte cache line");

                const std::vector<uint64_t> operands{10, 20, 30, 40, 50, 60};
                Instruction original(OpCode::LAMBDA, operands);
                require(original.numOperands() == operands.size(),
                    "variadic operand count was not retained");
                for (size_t i = 0; i < operands.size(); ++i)
                {
                    require(original.operandAt(i) == operands[i],
                        "tagged overflow changed a logical operand");
                }

                Instruction copied = original;
                original.setOperandAt(2, 300);
                original.setOperandAt(5, 600);
                require(copied.operandAt(2) == 30 && copied.operandAt(5) == 60,
                    "instruction copy shared its overflow allocation");

                Instruction moved = std::move(original);
                require(moved.operandAt(2) == 300 && moved.operandAt(5) == 600,
                    "instruction move lost tagged overflow ownership");

                Instruction assigned;
                assigned = copied;
                require(assigned.operandAt(4) == 50,
                    "instruction copy assignment lost an overflow operand");
                assigned = std::move(moved);
                require(assigned.operandAt(5) == 600,
                    "instruction move assignment lost an overflow operand");
                assigned.setSingleOperand(77);
                require(assigned.numOperands() == 1
                        && assigned.inlineOperands[0] == 77,
                    "replacing variadic operands did not restore inline form");

                Instruction aliased(OpCode::LAMBDA, operands);
                const auto* aliasedOverflow =
                    reinterpret_cast<const uint64_t*>(static_cast<uintptr_t>(
                        aliased.inlineOperands[2]));
                aliased.loadOperands(aliasedOverflow, 4);
                require(aliased.numOperands() == 4
                        && aliased.operandAt(0) == 30
                        && aliased.operandAt(1) == 40
                        && aliased.operandAt(2) == 50
                        && aliased.operandAt(3) == 60,
                    "self-aliased operand replacement read released storage");

                vm::bytecode::BytecodeProgram cacheProgram;
                for (size_t i = 0; i < 4096; ++i)
                    cacheProgram.emit(OpCode::NOP);
                auto* firstState = &cacheProgram.getOrCreateCachedState(3);
                firstState->fusedSlot = 91;
                (void)cacheProgram.getOrCreateCachedState(4095);
                require(cacheProgram.findCachedState(3) == firstState
                        && firstState->fusedSlot == 91,
                    "dense cached-state growth invalidated an existing entry");
                require(cacheProgram.findCachedState(2048) == nullptr,
                    "cold instruction unexpectedly allocated a cache record");
            });

        suite.addCallbackTest(
            "Function tiering scales conservatively with bytecode size", "",
            [](ScriptAPI&) {
                vm::bytecode::BytecodeProgram program;
                vm::jit::JitProfiler profiler(100);
                require(profiler.thresholdForBodySize(0) == 100
                        && profiler.thresholdForBodySize(32) == 100,
                    "unknown and small bodies must preserve the base threshold");
                require(profiler.thresholdForBodySize(288) == 75
                        && profiler.thresholdForBodySize(10000) == 75,
                    "large-function warm-up discount must cap at 25%");
                for (size_t i = 1; i < 100; ++i)
                {
                    require(!profiler.recordEntry(
                                program.getProgramId(), "small", 16),
                        "small function became hot before its base threshold");
                }
                require(profiler.recordEntry(
                            program.getProgramId(), "small", 16),
                    "small function must become hot at its base threshold");
                for (size_t i = 1; i < 75; ++i)
                {
                    require(!profiler.recordEntry(
                                program.getProgramId(), "large", 288),
                        "large function became hot before its scaled threshold");
                }
                require(profiler.recordEntry(
                            program.getProgramId(), "large", 288),
                    "large function must become hot at its scaled threshold");
                require(profiler.getEffectiveThreshold(vm::jit::FunctionId{
                            program.getProgramId(), "large"}) == 75,
                    "function profile must retain its selected threshold");
                vm::jit::JitProfiler customThreshold(3);
                require(!customThreshold.recordEntry(
                            program.getProgramId(), "custom")
                        && !customThreshold.recordEntry(
                            program.getProgramId(), "custom")
                        && customThreshold.recordEntry(
                            program.getProgramId(), "custom"),
                    "custom no-size threshold must trigger exactly at three calls");
            });

        suite.addCallbackTest(
            "Bytecode execution strategy owns and safely replaces programs", "",
            [](ScriptAPI& bootstrapApi) {
                auto environment = bootstrapApi.getEnvironment();
                auto localVm =
                    std::make_shared<vm::runtime::VirtualMachine>(environment);
                vm::compiler::BytecodeCompiler compiler(
                    environment, /*skipStrictValidation=*/true);
                services::ImportResolver importResolver(environment);
                services::ScriptAPI localApi(
                    environment, localVm.get(), nullptr);

                {
                    services::BytecodeExecutionStrategy strategy(
                        &compiler, localVm, &importResolver, &localApi);

                    ast::nodes::statements::ProgramNode firstAst;
                    (void)strategy.execute(&firstAst);
                    const auto* firstProgram = localVm->getProgram();
                    require(firstProgram
                            && localApi.getBytecodeProgram() == firstProgram
                            && firstProgram->isRuntimeAddressPinned()
                            && firstProgram->getInstructionCount() != 0,
                        "completed execution did not retain a live bound program");
                    require(localVm->getStats().instructionsExecuted != 0,
                        "post-execution statistics were reset too early");
                    const auto firstId = firstProgram->getProgramId();
                    const auto firstAddress =
                        reinterpret_cast<uintptr_t>(firstProgram);

                    ast::nodes::statements::ProgramNode secondAst;
                    (void)strategy.execute(&secondAst);
                    const auto* secondProgram = localVm->getProgram();
                    require(secondProgram
                            && reinterpret_cast<uintptr_t>(secondProgram)
                                != firstAddress
                            && secondProgram->getProgramId() != firstId
                            && localApi.getBytecodeProgram() == secondProgram
                            && !localVm->getLoadedPrograms().empty()
                            && localVm->getLoadedPrograms().front()
                                == secondProgram
                            && secondProgram->getInstructionCount() != 0,
                        "second execution did not safely replace and rebind "
                        "the owned program");
                }

                require(localVm->getProgram() == nullptr
                        && localApi.getBytecodeProgram() == nullptr,
                    "strategy destruction left dangling VM/API bindings");
            });

        suite.addCallbackTest(
            "Loop tiering uses dense program-qualified adaptive profiles", "",
            [](ScriptAPI&) {
                vm::bytecode::BytecodeProgram first;
                vm::bytecode::BytecodeProgram second;
                vm::jit::LoopProfiler profiler(100);
                require(profiler.thresholdForLoopSpan(0) == 100
                        && profiler.thresholdForLoopSpan(16) == 100,
                    "unknown and small loops must preserve the base threshold");
                require(profiler.thresholdForLoopSpan(80) == 75,
                    "large loops should receive only the capped 25% discount");
                const vm::jit::LoopId largeFirst{first.getProgramId(), 64};
                const vm::jit::LoopId sameOffsetSecond{
                    second.getProgramId(), 64};
                for (size_t i = 1; i < 75; ++i)
                {
                    require(!profiler.recordIteration(largeFirst, 80),
                        "large loop became hot before its adaptive threshold");
                }
                require(!profiler.recordIteration(sameOffsetSecond, 80),
                    "another program inherited loop hotness at the same offset");
                require(profiler.recordIteration(largeFirst, 80),
                    "large loop must tier at its adaptive threshold");
                const auto* firstProfile = profiler.getProfile(largeFirst);
                require(firstProfile && firstProfile->osrAttempted
                        && firstProfile->effectiveThreshold == 75,
                    "adaptive loop threshold must be retained in dense state");
                const auto* secondProfile =
                    profiler.getProfile(sameOffsetSecond);
                require(secondProfile && secondProfile->iterationCount == 1,
                    "second program must retain an independent dense profile");
                require(profiler.getProfiles().size() == 2,
                    "stats snapshot must expose every observed dense profile");
            });

        registerJitFrameAnalysisTests(suite);

        suite.addCallbackTest(
            "Method inlining rejects cross-program bytecode", "",
            [](ScriptAPI&) {
                vm::bytecode::BytecodeProgram callerProgram;
                vm::bytecode::BytecodeProgram libraryProgram;
                emitPushInt(libraryProgram, 17);
                libraryProgram.emit(OpCode::RETURN_VALUE);

                vm::bytecode::BytecodeProgram::FunctionMetadata callee{};
                callee.name = "Library::value/0";
                callee.startOffset = 0;
                callee.instructionCount = 2;
                callee.parameterCount = 1;
                callee.localCount = 1;
                callee.returnType = "int";
                libraryProgram.registerFunction(callee.name, callee);

                runtimeTypes::klass::ClassDefinition shape("Library");
                vm::jit::ic::MethodICEntry entry{};
                entry.shape = &shape;
                entry.funcMetadata = libraryProgram.getFunction(callee.name);
                entry.qualifiedName = callee.name;
                entry.program = &libraryProgram;

                vm::jit::ic::MethodInlineCache cache;
                require(cache.addEntry(entry),
                    "cross-program test IC entry must be cacheable");

                std::array<vm::optimization::InlineDecision,
                           vm::jit::ic::IC_MAX_POLYMORPHIC_ENTRIES> decisions{};
                const auto decision = vm::optimization::checkInlineEligibility(
                    callerProgram, cache, "Caller::run/0", 0, false,
                    &decisions);
                require(decision ==
                            vm::optimization::InlineDecision::CROSS_PROGRAM_CALLEE
                        && decisions[0] ==
                            vm::optimization::InlineDecision::CROSS_PROGRAM_CALLEE,
                    "library-program metadata must never be emitted through "
                    "the caller's bytecode stream");
            });

        suite.addCallbackTest(
            "Runtime optimization state is qualified by program identity", "",
            [](ScriptAPI&) {
                vm::bytecode::BytecodeProgram first;
                vm::bytecode::BytecodeProgram second;
                require(first.getProgramId() != second.getProgramId(),
                    "independent programs must have distinct identities");

                vm::bytecode::BytecodeProgram copied = first;
                require(copied.getProgramId() != first.getProgramId(),
                    "a copied program must receive an independent identity");
                const auto copiedId = copied.getProgramId();
                vm::bytecode::BytecodeProgram moved = std::move(copied);
                require(moved.getProgramId() == copiedId,
                    "program identity must follow bytecode across a move");

                vm::jit::JitProfiler functionProfiler(2);
                require(!functionProfiler.recordEntry(
                            first.getProgramId(), "sameName"),
                    "first program should remain cold after one call");
                require(!functionProfiler.recordEntry(
                            second.getProgramId(), "sameName"),
                    "second program must not inherit first-program hotness");
                require(functionProfiler.recordEntry(
                            first.getProgramId(), "sameName"),
                    "first program should tier independently");

                vm::jit::ic::InlineCacheTable table;
                table.getTypeFeedback(
                    first.getProgramId(), 7).executionCount = 9;
                require(table.getTypeFeedback(
                            second.getProgramId(), 7).executionCount == 0,
                    "equal offsets in different programs must not share feedback");

                vm::jit::LoopProfiler loopProfiler(2);
                const vm::jit::LoopId firstLoop{first.getProgramId(), 11};
                const vm::jit::LoopId secondLoop{second.getProgramId(), 11};
                require(!loopProfiler.recordIteration(firstLoop),
                    "first loop should remain cold after one backedge");
                require(!loopProfiler.recordIteration(secondLoop),
                    "second loop must not inherit first-loop backedges");
                require(loopProfiler.recordIteration(firstLoop),
                    "first loop should tier independently");

                vm::jit::JitCodeCache codeCache(/*maxCodeBytes=*/8);
                require(codeCache.canReserve(8) && !codeCache.canReserve(9),
                    "empty code-cache budget accounting is incorrect");
                const auto callee = first.internFrameName("sameName");
                asmjit::CodeHolder code;
                code.init(codeCache.getRuntime().environment());
                asmjit::x86::Assembler assembler(&code);
                assembler.ret();
                vm::jit::JitFunction nativeEntry = nullptr;
                require(codeCache.getRuntime().add(&nativeEntry, &code)
                            == asmjit::Error::kOk,
                    "test JIT entry allocation must succeed");
                require(codeCache.store(
                            first.getProgramId(), "sameName", nativeEntry,
                            /*codeBytes=*/8),
                    "first code-cache insertion must succeed");
                require(codeCache.byteSize() == 8 && !codeCache.canReserve(1),
                    "live native-code bytes did not consume the cache budget");
                codeCache.storeByIndex(
                    first.getProgramId(), 0, nativeEntry, callee);

                asmjit::CodeHolder duplicateCode;
                duplicateCode.init(codeCache.getRuntime().environment());
                asmjit::x86::Assembler duplicateAssembler(&duplicateCode);
                duplicateAssembler.ret();
                vm::jit::JitFunction duplicateEntry = nullptr;
                require(codeCache.getRuntime().add(
                            &duplicateEntry, &duplicateCode)
                            == asmjit::Error::kOk,
                    "duplicate test JIT entry allocation must succeed");
                require(!codeCache.store(
                            first.getProgramId(), "sameName", duplicateEntry),
                    "duplicate code-cache insertion must be rejected");
                require(codeCache.lookup(
                            first.getProgramId(), "sameName") == nativeEntry,
                    "duplicate insertion replaced the live compiled function");
                require(codeCache.lookupByIndex(
                            first.getProgramId(), 0).fn == nativeEntry,
                    "duplicate insertion changed the index fast path");
                require(codeCache.lookup(
                            second.getProgramId(), "sameName") == nullptr,
                    "same-named functions in different programs must not alias");
                require(codeCache.lookupByIndex(
                            second.getProgramId(), 0).fn == nullptr,
                    "equal function indices in different programs must not alias");
                const vm::jit::JitFunction invalidated =
                    codeCache.invalidate(vm::jit::FunctionId{
                        first.getProgramId(), "sameName"});
                require(invalidated == nativeEntry,
                    "program-qualified invalidation must release the owner");
                require(codeCache.byteSize() == 0 && codeCache.canReserve(8),
                    "invalidation did not return native-code budget");

                const vm::jit::FunctionId caller{
                    first.getProgramId(), "caller"};
                codeCache.registerInlineEdge(
                    first.getProgramId(), callee, caller);
                require(codeCache.invalidatedInlineCallersOf(
                            second.getProgramId(), callee).empty(),
                    "reverse inline edges must include callee program identity");
                require(codeCache.invalidatedInlineCallersOf(
                            first.getProgramId(), callee).size() == 1,
                    "owning program must retrieve its reverse inline edge");

                vm::jit::OSRManager osr;
                osr.getLoopProfiler().recordIteration(firstLoop);
                osr.reset();
                require(osr.getLoopProfiler().getProfiles().empty(),
                    "OSR reset must discard profiles and stale code domains");
            });
    }
}
