#pragma once
#include <vector>
#include <string>
#include <cstddef>
#include "SlotType.hpp"
#include "../../value/ValueType.hpp"

namespace vm::jit
{
    struct LocalSlotInfo
    {
        size_t slot;
        SlotType type;
        value::Value value;
    };

    struct OperandSlotInfo
    {
        SlotType type;
        value::Value value;
    };

    struct OSRState
    {
        // Loop identification
        size_t loopStartOffset = 0;
        size_t loopEndOffset = 0;
        size_t jumpBackOffset = 0;
        size_t loopConditionOffset = 0;

        // Captured locals
        std::vector<LocalSlotInfo> locals;
        size_t localCount = 0;

        // Captured operand stack (typically empty at JUMP_BACK)
        std::vector<OperandSlotInfo> operandStack;

        // Enclosing function info
        std::string functionName;

        // Where interpreter resumes after OSR loop exit
        size_t resumeOffset = 0;
    };

    struct OSRResult
    {
        bool success = false;
        bool deoptimized = false;
        size_t resumeIP = 0;
        std::vector<value::Value> updatedLocals;
    };
}
