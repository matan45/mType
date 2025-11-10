#include "ExceptionTable.hpp"
#include "../../runtimeTypes/klass/ObjectInstance.hpp"
#include <algorithm>

namespace vm::bytecode
{
    void ExceptionTable::addEntry(const ExceptionTableEntry& entry)
    {
        entries.push_back(entry);

        // Sort entries by nesting level (highest first) for correct precedence
        // When multiple entries cover the same IP, innermost (highest level) takes priority
        std::sort(entries.begin(), entries.end(),
            [](const ExceptionTableEntry& a, const ExceptionTableEntry& b)
            {
                // Primary sort: nesting level (descending)
                if (a.nestingLevel != b.nestingLevel)
                {
                    return a.nestingLevel > b.nestingLevel;
                }

                // Secondary sort: start IP (ascending) for stable ordering
                return a.startIP < b.startIP;
            });
    }

    const ExceptionTableEntry* ExceptionTable::findHandler(
        size_t ip,
        const std::string& exceptionTypeName,
        const value::Value& exceptionValue
    ) const
    {
        // Algorithm:
        // 1. Iterate through entries (already sorted by nesting level, innermost first)
        // 2. Check if entry covers the IP
        // 3. Check if entry's exception type matches thrown exception
        // 4. Prioritize CATCH handlers, but return FINALLY-only entries if no CATCH found

        for (const auto& entry : entries)
        {
            // Step 1: Check if this entry covers the instruction pointer
            if (!entry.covers(ip))
            {
                continue;
            }

            // Step 2: Check if this entry has a CATCH handler with matching type
            if (entry.hasCatchHandler())
            {
                if (isTypeCompatible(exceptionTypeName, entry.exceptionType, exceptionValue))
                {
                    // Found matching CATCH handler - return immediately
                    return &entry;
                }
            }

            // Step 3: FINALLY-only entries (FINALLY without CATCH)
            // Return immediately to respect nesting level - innermost FINALLY executes first
            // even if an outer scope has a CATCH handler
            if (entry.hasFinallyHandler() && !entry.hasCatchHandler())
            {
                // Return immediately - don't search outer scopes
                return &entry;
            }
        }

        // No handler found at all
        return nullptr;
    }

    bool ExceptionTable::isTypeCompatible(
        const std::string& exceptionType,
        const std::string& catchType,
        const value::Value& exceptionValue
    ) const
    {
        // Empty catchType means "catch all exceptions"
        if (catchType.empty())
        {
            return true;
        }

        // Exact type match
        if (exceptionType == catchType)
        {
            return true;
        }

        // Check inheritance: if exception is an ObjectInstance, use isInstanceOf()
        // This properly handles exception hierarchies (e.g., InitializationException extends Exception)
        if (std::holds_alternative<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(exceptionValue))
        {
            auto objInstance = std::get<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(exceptionValue);
            if (objInstance)
            {
                return objInstance->isInstanceOf(catchType);
            }
        }

        return false;
    }

    void ExceptionTable::updateFinallyIP(size_t startIP, size_t endIP, uint32_t nestingLevel, size_t finallyIP)
    {
        // Update all entries that match the given try block characteristics
        for (auto& entry : entries) {
            if (entry.startIP == startIP && entry.endIP == endIP && entry.nestingLevel == nestingLevel) {
                entry.finallyIP = finallyIP;
            }
        }
    }
}
