#include "BytecodeProgram.hpp"

#include <stdexcept>
#include <utility>
#include <vector>

namespace vm::bytecode
{
    namespace
    {
        bool isRuntimeFusion(OpCode opcode) noexcept
        {
            return opcode == OpCode::ADD_INT_CONST
                || opcode == OpCode::LOAD_LOCAL_CALL_CACHED
                || opcode == OpCode::LOAD_LOCAL_CALL_POLY_CACHED
                || opcode == OpCode::LOAD_LOCAL_GET_FIELD_CACHED;
        }

        OpCode coldOpcode(OpCode opcode) noexcept
        {
            switch (opcode)
            {
            case OpCode::CALL_METHOD_CACHED:
            case OpCode::CALL_METHOD_POLY_CACHED:
            case OpCode::LOAD_LOCAL_CALL_CACHED:
            case OpCode::LOAD_LOCAL_CALL_POLY_CACHED:
                return OpCode::CALL_METHOD;

            case OpCode::GET_FIELD_CACHED:
            case OpCode::LOAD_LOCAL_GET_FIELD_CACHED:
                return OpCode::GET_FIELD;
            case OpCode::SET_FIELD_CACHED:
                return OpCode::SET_FIELD;

            case OpCode::LOAD_VAR_CACHED:
                return OpCode::LOAD_VAR;
            case OpCode::STORE_VAR_CACHED:
                return OpCode::STORE_VAR;

            case OpCode::LOAD_LOCAL_INT:
            case OpCode::LOAD_LOCAL_FLOAT:
            case OpCode::LOAD_LOCAL_BOOL:
            case OpCode::LOAD_LOCAL_BOXED_INST:
                return OpCode::LOAD_LOCAL;
            case OpCode::STORE_LOCAL_INT:
            case OpCode::STORE_LOCAL_FLOAT:
            case OpCode::STORE_LOCAL_BOOL:
            case OpCode::STORE_LOCAL_BOXED_INST:
                return OpCode::STORE_LOCAL;

            case OpCode::BITWISE_AND_INT:
                return OpCode::BITWISE_AND_OP;
            case OpCode::BITWISE_OR_INT:
                return OpCode::BITWISE_OR_OP;
            case OpCode::BITWISE_XOR_INT:
                return OpCode::BITWISE_XOR_OP;
            case OpCode::LEFT_SHIFT_INT:
                return OpCode::LEFT_SHIFT_OP;
            case OpCode::RIGHT_SHIFT_INT:
                return OpCode::RIGHT_SHIFT_OP;
            case OpCode::BITWISE_NOT_INT:
                return OpCode::BITWISE_NOT_OP;

            case OpCode::ADD_INT_CONST:
                return OpCode::ADD_INT;
            default:
                return opcode;
            }
        }
    }

    BytecodeProgram::BytecodeProgram() : entryPoint(0) {}

    BytecodeProgram::BytecodeProgram(const BytecodeProgram& other)
        : constantPool(other.constantPool),
          functions(other.functions),
          functionIndexToName(other.functionIndexToName),
          functionNameToIndex(other.functionNameToIndex),
          frameNameById(other.frameNameById),
          sourceLocations(other.sourceLocations),
          classes(other.classes),
          interfaces(other.interfaces),
          annotationDeclarations(other.annotationDeclarations),
          staticInitializerFunctions(other.staticInitializerFunctions),
          globalVariables(other.globalVariables),
          globalExceptionTable(other.globalExceptionTable),
          entryPoint(other.entryPoint),
          topLevelLocalCount(other.topLevelLocalCount),
          topLevelLocalNames(other.topLevelLocalNames),
          sourceFilePath(other.sourceFilePath)
    {
        copySemanticInstructionsFrom(other);
        rebuildFrameNameIndexes();
    }

    ProgramIdentity BytecodeProgram::takeMovableIdentity(
        BytecodeProgram& other)
    {
        if (other.runtimeAddressPinned)
        {
            throw std::logic_error(
                "cannot move a BytecodeProgram after its runtime address "
                "has been pinned");
        }
        return std::move(other.identity);
    }

    BytecodeProgram::BytecodeProgram(BytecodeProgram&& other)
        : identity(takeMovableIdentity(other)),
          instructions(std::move(other.instructions)),
          constantPool(std::move(other.constantPool)),
          functions(std::move(other.functions)),
          functionIndexToName(std::move(other.functionIndexToName)),
          functionNameToIndex(std::move(other.functionNameToIndex)),
          frameNameById(std::move(other.frameNameById)),
          frameNameToId(std::move(other.frameNameToId)),
          frameNameToMeta(std::move(other.frameNameToMeta)),
          sourceLocations(std::move(other.sourceLocations)),
          classes(std::move(other.classes)),
          interfaces(std::move(other.interfaces)),
          annotationDeclarations(std::move(other.annotationDeclarations)),
          staticInitializerFunctions(std::move(other.staticInitializerFunctions)),
          globalVariables(std::move(other.globalVariables)),
          globalExceptionTable(std::move(other.globalExceptionTable)),
          entryPoint(other.entryPoint),
          topLevelLocalCount(other.topLevelLocalCount),
          topLevelLocalNames(std::move(other.topLevelLocalNames)),
          sourceFilePath(std::move(other.sourceFilePath)),
          fusionUnsafeTargets(std::move(other.fusionUnsafeTargets)),
          fusionUnsafeTargetsBuilt(other.fusionUnsafeTargetsBuilt),
          cachedStateIndices(std::move(other.cachedStateIndices)),
          cachedStates(std::move(other.cachedStates)),
          primitiveWrapperCache(std::move(other.primitiveWrapperCache)),
          objectConstructionCache(std::move(other.objectConstructionCache)),
          jitStableSlotPool(std::move(other.jitStableSlotPool))
    {
        // Rebuild non-owning string_view and FunctionMetadata indexes against
        // the destination containers. This avoids relying on allocator-
        // specific address preservation across deque/unordered_map moves.
        rebuildFrameNameIndexes();
        rebindMovedSelfPointers(&other);
    }

    BytecodeProgram& BytecodeProgram::operator=(const BytecodeProgram& other)
    {
        if (this == &other) return *this;
        if (runtimeAddressPinned)
        {
            throw std::logic_error(
                "cannot replace a BytecodeProgram after its runtime address "
                "has been pinned");
        }
        BytecodeProgram copy(other);
        return *this = std::move(copy);
    }

    BytecodeProgram& BytecodeProgram::operator=(BytecodeProgram&& other)
    {
        if (this == &other) return *this;
        if (runtimeAddressPinned || other.runtimeAddressPinned)
        {
            throw std::logic_error(
                "cannot move or replace a BytecodeProgram after its runtime "
                "address has been pinned");
        }

        identity = std::move(other.identity);
        instructions = std::move(other.instructions);
        constantPool = std::move(other.constantPool);
        functions = std::move(other.functions);
        functionIndexToName = std::move(other.functionIndexToName);
        functionNameToIndex = std::move(other.functionNameToIndex);
        frameNameById = std::move(other.frameNameById);
        frameNameToId = std::move(other.frameNameToId);
        frameNameToMeta = std::move(other.frameNameToMeta);
        sourceLocations = std::move(other.sourceLocations);
        classes = std::move(other.classes);
        interfaces = std::move(other.interfaces);
        annotationDeclarations = std::move(other.annotationDeclarations);
        staticInitializerFunctions = std::move(other.staticInitializerFunctions);
        globalVariables = std::move(other.globalVariables);
        globalExceptionTable = std::move(other.globalExceptionTable);
        entryPoint = other.entryPoint;
        topLevelLocalCount = other.topLevelLocalCount;
        topLevelLocalNames = std::move(other.topLevelLocalNames);
        sourceFilePath = std::move(other.sourceFilePath);
        fusionUnsafeTargets = std::move(other.fusionUnsafeTargets);
        fusionUnsafeTargetsBuilt = other.fusionUnsafeTargetsBuilt;
        cachedStateIndices = std::move(other.cachedStateIndices);
        cachedStates = std::move(other.cachedStates);
        primitiveWrapperCache = std::move(other.primitiveWrapperCache);
        objectConstructionCache = std::move(other.objectConstructionCache);
        jitStableSlotPool = std::move(other.jitStableSlotPool);

        rebuildFrameNameIndexes();
        rebindMovedSelfPointers(&other);
        return *this;
    }

    const BytecodeProgram::CachedInstructionState&
    BytecodeProgram::requireRuntimeFusionState(size_t fusedOffset) const
    {
        if (fusedOffset == 0 || fusedOffset >= instructions.size()
            || instructions[fusedOffset - 1].opcode != OpCode::NOP
            || !isRuntimeFusion(instructions[fusedOffset].opcode))
        {
            throw std::runtime_error(
                "runtime-fused bytecode has no matching predecessor at offset "
                + std::to_string(fusedOffset));
        }
        const auto* state = findCachedState(fusedOffset);
        if (!state)
        {
            throw std::runtime_error(
                "runtime-fused bytecode is missing its cached instruction state at offset "
                + std::to_string(fusedOffset));
        }
        return *state;
    }

    OpCode BytecodeProgram::semanticOpcodeAt(size_t offset) const
    {
        const auto& instruction = instructions.at(offset);
        if (instruction.opcode == OpCode::NOP && offset + 1 < instructions.size()
            && isRuntimeFusion(instructions[offset + 1].opcode))
        {
            (void)requireRuntimeFusionState(offset + 1);
            return instructions[offset + 1].opcode == OpCode::ADD_INT_CONST
                ? OpCode::PUSH_INT : OpCode::LOAD_LOCAL;
        }
        return coldOpcode(instruction.opcode);
    }

    size_t BytecodeProgram::semanticOperandCountAt(size_t offset) const
    {
        const auto& instruction = instructions.at(offset);
        if (instruction.opcode == OpCode::NOP && offset + 1 < instructions.size()
            && isRuntimeFusion(instructions[offset + 1].opcode))
        {
            (void)requireRuntimeFusionState(offset + 1);
            return 1;
        }
        if (isRuntimeFusion(instruction.opcode))
        {
            (void)requireRuntimeFusionState(offset);
        }
        return instruction.numOperands();
    }

    uint64_t BytecodeProgram::semanticOperandAt(
        size_t offset, size_t operandIndex) const
    {
        const auto& instruction = instructions.at(offset);
        if (instruction.opcode == OpCode::NOP && offset + 1 < instructions.size()
            && isRuntimeFusion(instructions[offset + 1].opcode))
        {
            if (operandIndex != 0)
                throw std::out_of_range(
                    "semantic instruction operand index out of range");
            return requireRuntimeFusionState(offset + 1).fusedSlot;
        }
        if (isRuntimeFusion(instruction.opcode))
        {
            (void)requireRuntimeFusionState(offset);
        }
        if (operandIndex >= instruction.numOperands())
            throw std::out_of_range(
                "semantic instruction operand index out of range");
        return instruction.operandAt(operandIndex);
    }

    void BytecodeProgram::copySemanticInstructionsFrom(
        const BytecodeProgram& other)
    {
        instructions = other.instructions;
        for (size_t offset = 0; offset < instructions.size(); ++offset)
        {
            auto& destination = instructions[offset];
            destination.opcode = other.semanticOpcodeAt(offset);
            const size_t operandCount = other.semanticOperandCountAt(offset);
            if (operandCount == destination.numOperands()) continue;

            if (operandCount == 0)
            {
                destination.clearOperands();
            }
            else if (operandCount == 1)
            {
                destination.setSingleOperand(
                    other.semanticOperandAt(offset, 0));
            }
            else
            {
                std::vector<uint64_t> operands;
                operands.reserve(operandCount);
                for (size_t i = 0; i < operandCount; ++i)
                    operands.push_back(other.semanticOperandAt(offset, i));
                destination.loadOperands(operands.data(), operands.size());
            }
        }
    }

    void BytecodeProgram::rebuildFrameNameIndexes()
    {
        frameNameToId.clear();
        frameNameToMeta.assign(frameNameById.size(), nullptr);
        frameNameToId.reserve(frameNameById.size());
        for (size_t i = 0; i < frameNameById.size(); ++i)
        {
            frameNameToId.emplace(std::string_view(frameNameById[i]),
                                  static_cast<uint32_t>(i));
            auto function = functions.find(frameNameById[i]);
            if (function != functions.end())
                frameNameToMeta[i] = &function->second;
        }
    }

    void BytecodeProgram::rebindMovedSelfPointers(
        const BytecodeProgram* oldAddress)
    {
        for (auto& state : cachedStates)
        {
            if (state.cachedProgram == oldAddress) state.cachedProgram = this;
            if (state.cachedMethodProgram == oldAddress)
                state.cachedMethodProgram = this;
            for (auto& cachedProgram : state.polyPrograms)
            {
                if (cachedProgram == oldAddress) cachedProgram = this;
            }
        }
    }
}
