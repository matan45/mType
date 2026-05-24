/**
 * VMVariableInspector — driver methods + frame-classification helpers.
 *
 * Hosts the small, always-shared surface of the inspector: ctor,
 * clearCache, formatValue, valueToDebugVariable, plus the
 * frame-classification helpers (isTopLevelScriptFrame,
 * collectTopLevelLocalSlots) that the split _Find.cpp and _Collect.cpp
 * translation units both need.
 *
 * Other responsibilities live in sibling TUs:
 *   - VMVariableInspector_Find.cpp    — findVariableValue + storage-kind lookups
 *   - VMVariableInspector_Collect.cpp — getLocalVariables / getGlobalVariables / getStaticVariables
 *   - VMVariableInspector_Format.cpp  — getVariableChildren / valueToString / getTypeName
 */

#include "VMVariableInspector.hpp"
#include <cstddef>
#include <algorithm>
#include "DebuggerConstants.hpp"
#include "../vm/bytecode/BytecodeProgram.hpp"
#include "../value/ValueShim.hpp"

namespace debugger
{
    namespace
    {
        bool isLocalStoreOpcode(vm::bytecode::OpCode opcode)
        {
            using vm::bytecode::OpCode;
            return opcode == OpCode::STORE_LOCAL ||
                   opcode == OpCode::STORE_LOCAL_INT ||
                   opcode == OpCode::STORE_LOCAL_FLOAT ||
                   opcode == OpCode::STORE_LOCAL_BOOL ||
                   opcode == OpCode::STORE_LOCAL_BOXED_INST;
        }
    }

    VMVariableInspector::VMVariableInspector()
        : nextRefId(constants::INITIAL_REFERENCE_ID)
    {
    }

    void VMVariableInspector::clearCache()
    {
        refIdToValue.clear();
        nextRefId = constants::INITIAL_REFERENCE_ID;
    }

    DebugVariable VMVariableInspector::formatValue(const std::string& name, const value::Value& val)
    {
        return valueToDebugVariable(name, val);
    }

    DebugVariable VMVariableInspector::valueToDebugVariable(const std::string& name, const value::Value& val)
    {
        // MYT-365: value-class instances expand to their fields, EXCEPT for the
        // boxed-primitive wrappers (Int/Float/Bool/String) — those render as a
        // leaf showing the inner primitive directly, so an expansion arrow that
        // only reveals `value = 42` would be noise. The boxed-wrapper exclusion
        // covers both OBJECT and VALUE_OBJECT shapes for consistency.
        bool isWrapper = isBoxedPrimitiveWrapper(val);
        bool expandable = value::isNativeArray(val)
            || (value::isAnyObject(val) && !isWrapper)
            || (value::isValueObject(val) && !isWrapper)
            || value::isFlatMultiArray(val)
            || value::isSparseMultiArray(val)
            || value::isFlatMultiObjectArray(val);

        int64_t refId = 0;
        if (expandable)
        {
            refId = nextRefId++;
            refIdToValue[refId] = val;
        }

        return DebugVariable(name, valueToString(val), getTypeName(val), expandable, refId);
    }

    bool VMVariableInspector::isTopLevelScriptFrame(
        std::shared_ptr<vm::runtime::VirtualMachine> vm,
        const vm::runtime::CallFrame& frame)
    {
        const auto* program = vm ? vm->getProgram() : nullptr;
        return program &&
               frame.localBase == 0 &&
               program->getFrameName(frame.functionName) == "__script_main__";
    }

    std::unordered_map<std::string, size_t>
    VMVariableInspector::collectTopLevelLocalSlots(std::shared_ptr<vm::runtime::VirtualMachine> vm)
    {
        std::unordered_map<std::string, size_t> slots;
        if (!vm)
        {
            return slots;
        }

        const auto* program = vm->getProgram();
        if (!program)
        {
            return slots;
        }

        const auto& topLevelNames = program->getTopLevelLocalNames();
        for (size_t slot = 0; slot < topLevelNames.size(); ++slot)
        {
            if (!topLevelNames[slot].empty())
            {
                slots[topLevelNames[slot]] = slot;
            }
        }

        const size_t instructionCount = program->getInstructionCount();
        if (!slots.empty() || instructionCount == 0)
        {
            return slots;
        }

        // Fallback: scan from program entry to current PC and recover names
        // from STORE_LOCAL operand metadata when explicit debug names are
        // absent.
        const size_t currentIp = std::min(vm->getInstructionPointer(), instructionCount - 1);
        const size_t startIp = std::min(program->getEntryPoint(), currentIp);

        for (size_t ip = startIp; ip <= currentIp; ++ip)
        {
            const auto& instr = program->getInstruction(ip);
            if (!isLocalStoreOpcode(instr.opcode) || instr.numOperands() < 2)
            {
                continue;
            }

            const size_t slot = static_cast<size_t>(instr.inlineOperands[0]);
            const size_t nameIndex = static_cast<size_t>(instr.inlineOperands[1]);
            if (slot >= program->getTopLevelLocalCount())
            {
                continue;
            }

            try
            {
                const std::string& name = program->getConstantPool().getString(nameIndex);
                if (!name.empty())
                {
                    slots[name] = slot;
                }
            }
            catch (...)
            {
                // Ignore malformed debug metadata and keep inspecting other stores.
            }
        }

        return slots;
    }
} // namespace debugger
