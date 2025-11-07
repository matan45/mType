#pragma once

#include <vector>
#include <string>
#include <cstddef>

namespace vm::bytecode
{
    /**
     * Exception Table Entry - Represents a protected region and its exception handlers
     *
     * Each entry defines a bytecode region [startIP, endIP) and specifies which handlers
     * (CATCH and/or FINALLY) apply to exceptions thrown in that region.
     *
     * Design principles:
     * - Compile-time construction: Built during bytecode compilation
     * - O(k) lookup: Finding handlers requires checking k entries (typically 3-5 per function)
     * - Nesting level: Pre-computed priority for nested try-catch blocks
     * - Type filtering: Supports exception type inheritance matching
     */
    struct ExceptionTableEntry
    {
        // Protected bytecode region [startIP, endIP)
        size_t startIP;
        size_t endIP;

        // Handler instruction pointers (SIZE_MAX if handler doesn't exist)
        size_t catchIP;      // Points to CATCH instruction
        size_t finallyIP;    // Points to FINALLY instruction

        // Exception type filter (empty string = catch all exceptions)
        std::string exceptionType;

        // Nesting level: Inner blocks have higher values (for prioritization)
        uint32_t nestingLevel;

        // Catch variable binding information
        std::string catchVarName;  // Name of exception variable in catch block
        size_t catchVarSlot;       // Local variable slot for exception (SIZE_MAX if none)

        /**
         * Constructor for exception table entry
         */
        ExceptionTableEntry(
            size_t start,
            size_t end,
            size_t catchIP,
            size_t finallyIP,
            const std::string& exType,
            uint32_t level,
            const std::string& varName = "",
            size_t varSlot = SIZE_MAX
        ) : startIP(start)
          , endIP(end)
          , catchIP(catchIP)
          , finallyIP(finallyIP)
          , exceptionType(exType)
          , nestingLevel(level)
          , catchVarName(varName)
          , catchVarSlot(varSlot)
        {
        }

        /**
         * Check if an instruction pointer is within this entry's protected region
         */
        bool covers(size_t ip) const
        {
            return ip >= startIP && ip < endIP;
        }

        /**
         * Check if this entry has a catch handler
         */
        bool hasCatchHandler() const
        {
            return catchIP != SIZE_MAX;
        }

        /**
         * Check if this entry has a finally handler
         */
        bool hasFinallyHandler() const
        {
            return finallyIP != SIZE_MAX;
        }
    };

    /**
     * Exception Table - Collection of exception handlers for a function/lambda
     *
     * Provides O(k) exception handler lookup where k is the number of entries.
     * Entries are ordered by nesting level (innermost first) for correct precedence.
     *
     * Usage:
     * - Compiler: Call addEntry() during try-catch-finally compilation
     * - Runtime: Call findHandler() when exception is thrown
     */
    class ExceptionTable
    {
    private:
        std::vector<ExceptionTableEntry> entries;

    public:
        /**
         * Add an exception table entry
         * Entries are automatically sorted by nesting level (highest first)
         */
        void addEntry(const ExceptionTableEntry& entry);

        /**
         * Find an exception handler for the given instruction pointer and exception type
         *
         * Algorithm:
         * 1. Filter entries that cover the given IP
         * 2. Sort by nesting level (innermost first)
         * 3. Check type compatibility with exception type hierarchy
         * 4. Return first matching entry
         *
         * @param ip Current instruction pointer where exception was thrown
         * @param exceptionTypeName Fully qualified exception type name
         * @return Pointer to matching entry, or nullptr if no handler found
         */
        const ExceptionTableEntry* findHandler(
            size_t ip,
            const std::string& exceptionTypeName
        ) const;

        /**
         * Get all exception table entries
         */
        const std::vector<ExceptionTableEntry>& getEntries() const
        {
            return entries;
        }

        /**
         * Check if table is empty
         */
        bool isEmpty() const
        {
            return entries.empty();
        }

        /**
         * Get number of entries
         */
        size_t size() const
        {
            return entries.size();
        }

        /**
         * Clear all entries
         */
        void clear()
        {
            entries.clear();
        }

        /**
         * Update finally IP for entries matching the given try block
         * Used when a finally block is added to a try that already has catch blocks
         *
         * @param startIP Start of protected region
         * @param endIP End of protected region
         * @param nestingLevel Nesting level to match
         * @param finallyIP Finally handler offset to set
         */
        void updateFinallyIP(size_t startIP, size_t endIP, uint32_t nestingLevel, size_t finallyIP);

    private:
        /**
         * Check if exceptionType matches or is a subclass of catchType
         *
         * Type matching rules:
         * - Empty catchType matches all exceptions
         * - Exact type match always succeeds
         * - TODO: Support exception type inheritance hierarchy
         *
         * @param exceptionType The type of the thrown exception
         * @param catchType The type filter in the catch block
         * @return true if exception can be caught by this handler
         */
        bool isTypeCompatible(
            const std::string& exceptionType,
            const std::string& catchType
        ) const;
    };
}
