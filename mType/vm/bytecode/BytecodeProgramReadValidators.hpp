#pragma once
#include <cstddef>
#include <stdexcept>
#include <string>
#include "../../constants/SecurityConstants.hpp"

namespace vm::bytecode::readv
{
    // Sentinel for "no handler" used by the bytecode compiler. SIZE_MAX
    // means the catch / finally branch is absent and must not be range-
    // checked against the instruction array.
    inline constexpr size_t IP_NONE = static_cast<size_t>(-1);

    // Reject malformed counts read from a bytecode stream before they reach
    // a vector::resize. Throws so the caller propagates a clear "bytecode
    // rejected" message.
    inline void validateCount(size_t count, size_t max, const char* what) {
        if (count > max) {
            throw std::runtime_error(
                std::string("Bytecode deserialization rejected: ") + what +
                " count " + std::to_string(count) + " exceeds limit " + std::to_string(max));
        }
    }

    // Cap individual serialized strings at MAX_STRING_BYTES so an attacker
    // cannot allocate a multi-GiB std::string from a 16-byte length prefix.
    inline void validateStringLen(size_t len, const char* what) {
        if (len > constants::security::MAX_STRING_BYTES) {
            throw std::runtime_error(
                std::string("Bytecode deserialization rejected: ") + what +
                " string length " + std::to_string(len) + " exceeds limit " +
                std::to_string(constants::security::MAX_STRING_BYTES));
        }
    }

    // Verify that an exception-table entry's instruction pointers fall within
    // the loaded instruction array. Skips the SIZE_MAX sentinel for absent
    // catch / finally branches.
    inline void validateExceptionEntry(size_t startIP, size_t endIP,
                                       size_t catchIP, size_t finallyIP,
                                       size_t instructionCount,
                                       const char* context) {
        if (startIP >= endIP) {
            throw std::runtime_error(
                std::string("Bytecode deserialization rejected: ") + context +
                " exception entry has startIP " + std::to_string(startIP) +
                " >= endIP " + std::to_string(endIP));
        }
        if (endIP > instructionCount) {
            throw std::runtime_error(
                std::string("Bytecode deserialization rejected: ") + context +
                " exception entry endIP " + std::to_string(endIP) +
                " exceeds instruction count " + std::to_string(instructionCount));
        }
        if (catchIP != IP_NONE && catchIP >= instructionCount) {
            throw std::runtime_error(
                std::string("Bytecode deserialization rejected: ") + context +
                " exception entry catchIP " + std::to_string(catchIP) +
                " out of range (instruction count " + std::to_string(instructionCount) + ")");
        }
        if (finallyIP != IP_NONE && finallyIP >= instructionCount) {
            throw std::runtime_error(
                std::string("Bytecode deserialization rejected: ") + context +
                " exception entry finallyIP " + std::to_string(finallyIP) +
                " out of range (instruction count " + std::to_string(instructionCount) + ")");
        }
    }
}
