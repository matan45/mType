#include "JitFrameAnalysis_Internal.hpp"

#include <algorithm>
#include <array>
#include <limits>

namespace vm::jit::analysis::detail
{
    bool TypedCfgAnalyzer::collectInlineRequirements(
        const Instruction& instruction,
        size_t ip,
        size_t inputDepth,
        size_t inlineDepth,
        FrameAnalysisResult& result)
    {
        if (!usesBoxedTypes) return true;
        switch (instruction.opcode)
        {
            case OpCode::CALL: case OpCode::CALL_FAST:
            case OpCode::CALL_STATIC:
                return collectPlainInline(
                    instruction, ip, inputDepth, inlineDepth, result);
            case OpCode::CALL_METHOD:
            case OpCode::CALL_METHOD_CACHED:
            case OpCode::CALL_METHOD_POLY_CACHED:
                return collectMethodInline(
                    instruction, ip, inputDepth, inlineDepth, result);
            case OpCode::LOAD_LOCAL_CALL_CACHED:
            case OpCode::LOAD_LOCAL_CALL_POLY_CACHED:
                return collectMethodInline(
                    instruction, ip, inputDepth + 1, inlineDepth, result);
            default:
                return true;
        }
    }

    bool TypedCfgAnalyzer::collectPlainInline(
        const Instruction& instruction,
        size_t ip,
        size_t inputDepth,
        size_t inlineDepth,
        FrameAnalysisResult& result)
    {
        if (!hasOperands(instruction, 2)) return true;
        const auto* callee = instruction.opcode == OpCode::CALL_FAST
            ? program.getFunctionByIndex(
                static_cast<size_t>(instruction.inlineOperands[0]))
            : namedCallee(instruction);
        if (!callee) return true;
        const size_t argCount = static_cast<size_t>(
            instruction.inlineOperands[1]);
        if (callee->parameterCount != argCount) return true;
        if (instruction.opcode == OpCode::CALL_STATIC &&
            !staticCallSignatureInlineable(*callee))
            return true;

        const auto decision = optimization::checkFunctionInlineEligibility(
            program, *callee, currentCompilingFunction, inlineDepth);
        if (decision != optimization::InlineDecision::INLINE) return true;
        if (inputDepth < argCount) return true;
        return mergeInlineCandidate(
            *callee, inputDepth - argCount,
            inlineDepth + 1, result, ip);
    }

    bool TypedCfgAnalyzer::collectMethodInline(
        const Instruction& instruction,
        size_t ip,
        size_t dispatchDepth,
        size_t inlineDepth,
        FrameAnalysisResult& result)
    {
        if (!typeFeedback || !hasOperands(instruction, 2)) return true;
        auto& table = typeFeedback->getICTable();
        if (!table.hasMethodIC(program.getProgramId(), ip)) return true;
        auto& cache = table.getMethodIC(program.getProgramId(), ip);

        std::array<optimization::InlineDecision,
                   ic::IC_MAX_POLYMORPHIC_ENTRIES> decisions{};
        decisions.fill(optimization::InlineDecision::INLINE);
        const auto siteDecision = optimization::checkInlineEligibility(
            program, cache, currentCompilingFunction, inlineDepth,
            isOSRCompilation, &decisions);
        if (siteDecision != optimization::InlineDecision::INLINE)
            return true;

        const size_t argCount = static_cast<size_t>(
            instruction.inlineOperands[1]);
        if (argCount >= dispatchDepth) return true;
        const size_t calleeStackBase = dispatchDepth - argCount - 1;
        for (uint8_t i = 0; i < cache.entryCount; ++i)
        {
            if (!isInlineableDecision(decisions[i])) continue;
            const auto& entry = cache.entries[i];
            if (entry.program && entry.program != &program)
            {
                setFailure(
                    result, FrameAnalysisStatus::CROSS_PROGRAM_INLINE, ip);
                return false;
            }
            const auto* callee = static_cast<const
                bytecode::BytecodeProgram::FunctionMetadata*>(
                    entry.funcMetadata);
            if (!callee || callee->parameterCount != argCount + 1)
                continue;
            if (!mergeInlineCandidate(
                    *callee, calleeStackBase, inlineDepth + 1,
                    result, ip))
                return false;
        }
        return true;
    }

    bool TypedCfgAnalyzer::mergeInlineCandidate(
        const bytecode::BytecodeProgram::FunctionMetadata& callee,
        size_t dispatchDepth,
        size_t nestedInlineDepth,
        FrameAnalysisResult& result,
        size_t callSite)
    {
        FrameAnalysisResult nested = analyzeFunction(
            callee, nestedInlineDepth,
            /*owningFrameMinimumLocal=*/false);
        if (!nested.proven())
        {
            // A callee incompatible with the lexical emitter remains a generic
            // call. The emit-time guard repeats the analysis before inlining.
            if (nested.status ==
                    FrameAnalysisStatus::EMITTER_STACK_MODEL_MISMATCH ||
                nested.status ==
                    FrameAnalysisStatus::INLINE_FRAME_LIMIT_EXCEEDED)
                return true;
            setFailure(
                result, nested.status,
                nested.offendingOffset == std::numeric_limits<size_t>::max()
                    ? callSite : nested.offendingOffset);
            return false;
        }
        if (callee.localCount > hardInlineLocalLimit ||
            nested.inlineLocalSlots >
                hardInlineLocalLimit - callee.localCount)
            return true;
        result.operandStackPeak = std::max(
            result.operandStackPeak,
            saturatedAdd(dispatchDepth, nested.operandStackPeak));
        result.inlineLocalSlots = std::max(
            result.inlineLocalSlots,
            saturatedAdd(callee.localCount, nested.inlineLocalSlots));
        result.typeWideningCount += nested.typeWideningCount;
        return true;
    }

    const bytecode::BytecodeProgram::FunctionMetadata*
    TypedCfgAnalyzer::namedCallee(const Instruction& instruction) const
    {
        if (!hasOperands(instruction, 1)) return nullptr;
        const uint32_t nameIndex = static_cast<uint32_t>(
            instruction.inlineOperands[0]);
        if (nameIndex >= program.getConstantPool().strings.size())
            return nullptr;
        return program.getFunction(
            program.getConstantPool().getString(nameIndex));
    }

    bool TypedCfgAnalyzer::staticCallSignatureInlineable(
        const bytecode::BytecodeProgram::FunctionMetadata& callee)
    {
        if (!callee.genericTypeParameters.empty()) return false;
        for (const std::string& type : callee.parameterTypes)
        {
            if (type != "int" && type != "float" && type != "bool")
                return false;
        }
        return callee.returnType == "int" ||
               callee.returnType == "float" ||
               callee.returnType == "bool";
    }

    size_t TypedCfgAnalyzer::saturatedAdd(
        size_t left, size_t right) noexcept
    {
        return left > std::numeric_limits<size_t>::max() - right
            ? std::numeric_limits<size_t>::max()
            : left + right;
    }
}
