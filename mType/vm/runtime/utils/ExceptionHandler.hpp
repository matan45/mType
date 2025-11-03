#pragma once
#include <vector>
#include <string>
#include "../../bytecode/BytecodeProgram.hpp"
#include "../../bytecode/OpCode.hpp"
#include "../context/ExecutionContext.hpp"
#include "../../../errors/UserException.hpp"
#include "../stack/StackManager.hpp"

namespace vm::runtime::utils
{
    /**
     * Helper class for handling exceptions in the virtual machine
     * Extracted from VirtualMachine::interpretLoop() to improve maintainability
     * Handles: exception matching, catch block search, call frame unwinding
     */
    class ExceptionHandler
    {
    public:
        /**
         * Result of exception handling attempt
         */
        struct HandlingResult {
            bool handled;               // Was exception handled by a catch/finally block?
            size_t newInstructionPointer; // Where to resume execution (if handled)
            bool jumpedToFinally;       // True if jumped to FINALLY (needs re-throw after), false if jumped to CATCH
        };

        explicit ExceptionHandler(const bytecode::BytecodeProgram* program,
                                 std::shared_ptr<StackManager> stackMgr,
                                 std::vector<CallFrame>& callStack);
        ~ExceptionHandler() = default;

        /**
         * Handle a user exception by searching for matching catch blocks
         * @param e The user exception to handle
         * @param currentIP Current instruction pointer where exception occurred
         * @param currentFinallyOffset Offset of currently executing finally block (SIZE_MAX if none)
         * @return HandlingResult indicating if exception was handled and where to resume
         */
        HandlingResult handleUserException(const errors::UserException& e,
                                          size_t currentIP,
                                          size_t currentFinallyOffset);

    private:
        const bytecode::BytecodeProgram* program;
        std::shared_ptr<StackManager> stackManager;
        std::vector<CallFrame>& callStack;

        // Helper methods
        size_t determineSearchLimit(size_t currentIP) const;
        bool searchForCatchBlock(const errors::UserException& e,
                                size_t startIP,
                                size_t searchLimit,
                                size_t& catchIP) const;
        void unwindCallFrames(size_t targetIP);
        bool isInFinallyBlock(size_t catchIP, size_t currentFinallyOffset) const;
        void cleanupStack(size_t targetFrameBase);
    };
}
