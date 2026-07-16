#pragma once

#include "JitFrameAnalysis.hpp"
#include "../ic/TypeFeedbackCollector.hpp"
#include "../../optimization/InlineAnalysis.hpp"

#include <cstddef>
#include <cstdint>
#include <deque>
#include <string>
#include <vector>

namespace vm::jit::analysis::detail
{
    enum class AbstractSlotType : uint8_t
    {
        UNKNOWN,
        INT,
        FLOAT,
        BOOL,
        BOXED,
    };

    struct AbstractState
    {
        std::vector<AbstractSlotType> stack;
        std::vector<AbstractSlotType> locals;
    };

    AbstractSlotType typeFromName(const std::string& type);
    AbstractSlotType forcedLocalType(bytecode::OpCode opcode);
    bool hasOperands(
        const bytecode::BytecodeProgram::Instruction& instruction,
        size_t required) noexcept;
    bool popValues(AbstractState& state, size_t count);
    bool replaceValues(
        AbstractState& state, size_t consumed, AbstractSlotType produced);
    bool requireTop(const AbstractState& state) noexcept;

    class TypedCfgAnalyzer
    {
    public:
        TypedCfgAnalyzer(
            const bytecode::BytecodeProgram& program,
            const ic::TypeFeedbackCollector* typeFeedback,
            bool usesBoxedTypes,
            std::string currentCompilingFunction,
            bool isOSRCompilation,
            size_t hardInlineLocalLimit);

        FrameAnalysisResult analyzeFunction(
            const bytecode::BytecodeProgram::FunctionMetadata& function,
            size_t inlineDepth = 0,
            bool owningFrameMinimumLocal = true);
        FrameAnalysisResult analyzeOSRRange(
            size_t startOffset,
            size_t endOffsetExclusive,
            size_t localCount);

    private:
        using Instruction = bytecode::BytecodeProgram::Instruction;
        using OpCode = bytecode::OpCode;

        struct PendingState
        {
            bool initialized = false;
            bool queued = false;
            AbstractState state;
        };

        const bytecode::BytecodeProgram& program;
        const ic::TypeFeedbackCollector* typeFeedback;
        bool usesBoxedTypes;
        std::string currentCompilingFunction;
        bool isOSRCompilation;
        size_t hardInlineLocalLimit;
        bool allowExternalBranchTargets = false;

        static bool isInlineableDecision(
            optimization::InlineDecision decision) noexcept;
        static void setFailure(
            FrameAnalysisResult& result,
            FrameAnalysisStatus status,
            size_t offset);
        bool mergeState(
            PendingState& destination,
            const AbstractState& source,
            FrameAnalysisResult& result,
            size_t targetOffset);
        static bool widenTypes(
            std::vector<AbstractSlotType>& destination,
            const std::vector<AbstractSlotType>& source,
            FrameAnalysisResult& result);
        bool enqueue(
            std::vector<PendingState>& states,
            std::deque<size_t>& worklist,
            size_t rangeStart,
            size_t rangeEnd,
            size_t target,
            const AbstractState& state,
            FrameAnalysisResult& result);
        FrameAnalysisResult analyzeRange(
            size_t rangeStart,
            size_t rangeEnd,
            AbstractState entry,
            size_t inlineDepth,
            bool externalBranchTargetsAllowed);
        static void updateBasePeak(
            FrameAnalysisResult& result, size_t depth);

        bool validateLinearEmitterModel(
            size_t rangeStart,
            size_t rangeEnd,
            const std::vector<PendingState>& cfgStates,
            AbstractState linearState,
            FrameAnalysisResult& result);
        bool applyLinearEmitterInstruction(
            const Instruction& instruction,
            size_t ip,
            size_t rangeStart,
            size_t rangeEnd,
            AbstractState& state,
            FrameAnalysisResult& result);
        bool validateLinearBranch(
            const Instruction& instruction,
            size_t ip,
            size_t rangeStart,
            size_t rangeEnd,
            FrameAnalysisResult& result) const;
        static bool failEmitterModel(
            FrameAnalysisResult& result, size_t ip);
        void processInstruction(
            const Instruction& instruction,
            size_t ip,
            const AbstractState& input,
            size_t rangeStart,
            size_t rangeEnd,
            std::vector<PendingState>& states,
            std::deque<size_t>& worklist,
            FrameAnalysisResult& result);
        bool processControlInstruction(
            const Instruction& instruction,
            size_t ip,
            const AbstractState& input,
            size_t rangeStart,
            size_t rangeEnd,
            std::vector<PendingState>& states,
            std::deque<size_t>& worklist,
            FrameAnalysisResult& result);
        bool processUnconditionalBranch(
            const Instruction& instruction,
            const AbstractState& input,
            size_t rangeStart,
            size_t rangeEnd,
            std::vector<PendingState>& states,
            std::deque<size_t>& worklist,
            FrameAnalysisResult& result,
            size_t ip);
        bool processPoppingBranch(
            const Instruction& instruction,
            const AbstractState& input,
            size_t rangeStart,
            size_t rangeEnd,
            std::vector<PendingState>& states,
            std::deque<size_t>& worklist,
            FrameAnalysisResult& result,
            size_t ip);
        bool processShortCircuitBranch(
            const Instruction& instruction,
            const AbstractState& input,
            size_t rangeStart,
            size_t rangeEnd,
            std::vector<PendingState>& states,
            std::deque<size_t>& worklist,
            FrameAnalysisResult& result,
            size_t ip);

        bool transferInstruction(
            const Instruction& instruction,
            AbstractState& state,
            size_t ip,
            FrameAnalysisResult& result);
        bool transferStackAndArithmetic(
            const Instruction& instruction,
            AbstractState& state,
            size_t ip,
            FrameAnalysisResult& result);
        bool transferArithmeticOperation(
            OpCode opcode,
            AbstractState& state,
            size_t ip,
            FrameAnalysisResult& result);
        static bool isIntBinary(OpCode opcode) noexcept;
        static bool isFloatBinary(OpCode opcode) noexcept;
        static bool isGenericBinary(OpCode opcode) noexcept;
        static bool isComparison(OpCode opcode) noexcept;
        static bool isLogicalBinary(OpCode opcode) noexcept;
        static bool isIntUnary(OpCode opcode) noexcept;
        bool transferAddStoreLocal(
            AbstractState& state,
            size_t ip,
            FrameAnalysisResult& result);
        bool transferLocalsAndCalls(
            const Instruction& instruction,
            AbstractState& state,
            size_t ip,
            FrameAnalysisResult& result);
        bool transferLoadLocal(
            const Instruction& instruction,
            AbstractState& state,
            size_t ip,
            FrameAnalysisResult& result);
        bool transferStoreLocal(
            const Instruction& instruction,
            AbstractState& state,
            size_t ip,
            FrameAnalysisResult& result);
        bool transferLoadStoreLocal(
            const Instruction& instruction,
            AbstractState& state,
            size_t ip,
            FrameAnalysisResult& result);
        bool transferNamedCall(
            const Instruction& instruction,
            AbstractState& state,
            size_t ip,
            FrameAnalysisResult& result);
        bool transferFastCall(
            const Instruction& instruction,
            AbstractState& state,
            size_t ip,
            FrameAnalysisResult& result);
        bool transferStaticCall(
            const Instruction& instruction,
            AbstractState& state,
            size_t ip,
            FrameAnalysisResult& result);
        bool transferMethodCall(
            const Instruction& instruction,
            AbstractState& state,
            size_t ip,
            FrameAnalysisResult& result,
            bool fusedLocalLoad);
        static bool transferResolvedCall(
            AbstractState& state,
            size_t args,
            const std::string& returnType,
            size_t ip,
            FrameAnalysisResult& result);

        bool transferObjectsAndArrays(
            const Instruction& instruction,
            AbstractState& state,
            size_t ip,
            FrameAnalysisResult& result);
        bool transferFieldOperation(
            const Instruction& instruction,
            AbstractState& state,
            size_t ip,
            FrameAnalysisResult& result);
        bool transferObjectOperation(
            const Instruction& instruction,
            AbstractState& state,
            size_t ip,
            FrameAnalysisResult& result);
        bool transferObjectCreation(
            const Instruction& instruction,
            AbstractState& state,
            size_t ip,
            FrameAnalysisResult& result);
        bool transferArrayOperation(
            const Instruction& instruction,
            AbstractState& state,
            size_t ip,
            FrameAnalysisResult& result);
        bool transferMultiArray(
            const Instruction& instruction,
            AbstractState& state,
            size_t ip,
            FrameAnalysisResult& result);
        bool transferPrimitiveInvoke(
            const Instruction& instruction,
            AbstractState& state,
            size_t ip,
            FrameAnalysisResult& result);
        static bool isPrimitiveIntBinary(OpCode opcode) noexcept;
        static bool isPrimitiveFloatBinary(OpCode opcode) noexcept;
        static bool isPrimitiveBoolBinary(OpCode opcode) noexcept;
        static bool isPrimitiveComparison(OpCode opcode) noexcept;
        static bool isPrimitiveUnary(OpCode opcode) noexcept;
        static AbstractSlotType primitiveUnaryOutput(
            OpCode opcode) noexcept;

        bool collectInlineRequirements(
            const Instruction& instruction,
            size_t ip,
            size_t inputDepth,
            size_t inlineDepth,
            FrameAnalysisResult& result);
        bool collectPlainInline(
            const Instruction& instruction,
            size_t ip,
            size_t inputDepth,
            size_t inlineDepth,
            FrameAnalysisResult& result);
        bool collectMethodInline(
            const Instruction& instruction,
            size_t ip,
            size_t dispatchDepth,
            size_t inlineDepth,
            FrameAnalysisResult& result);
        bool mergeInlineCandidate(
            const bytecode::BytecodeProgram::FunctionMetadata& callee,
            size_t dispatchDepth,
            size_t nestedInlineDepth,
            FrameAnalysisResult& result,
            size_t callSite);
        const bytecode::BytecodeProgram::FunctionMetadata* namedCallee(
            const Instruction& instruction) const;
        static bool staticCallSignatureInlineable(
            const bytecode::BytecodeProgram::FunctionMetadata& callee);
        static size_t saturatedAdd(size_t left, size_t right) noexcept;

        static bool pushWithOperands(
            const Instruction& instruction,
            AbstractState& state,
            AbstractSlotType type,
            size_t ip,
            FrameAnalysisResult& result);
        static bool requireOrFail(
            const AbstractState& state,
            size_t ip,
            FrameAnalysisResult& result);
        static bool popOrFail(
            AbstractState& state,
            size_t count,
            size_t ip,
            FrameAnalysisResult& result);
        static bool replaceOrFail(
            AbstractState& state,
            size_t consumed,
            AbstractSlotType produced,
            size_t ip,
            FrameAnalysisResult& result);
        static bool setLocal(
            AbstractState& state,
            uint64_t slotValue,
            AbstractSlotType type,
            size_t ip,
            FrameAnalysisResult& result);
        static bool failUnderflow(
            FrameAnalysisResult& result, size_t ip);
        static bool failOperands(
            FrameAnalysisResult& result, size_t ip);
        static bool failLocal(
            FrameAnalysisResult& result, size_t ip);
        static bool failCallee(
            FrameAnalysisResult& result, size_t ip);
    };
}
