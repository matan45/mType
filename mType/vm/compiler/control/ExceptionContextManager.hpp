#pragma once
#include <vector>
#include <string>
#include <cstddef>
#include <cstdint>

namespace vm::compiler::control
{
    /**
     * Manages exception handling context for try/catch/finally compilation
     * Tracks jump offsets and exception handlers that need to be patched
     */
    class ExceptionContextManager
    {
    public:
        struct CatchHandler {
            std::string exceptionType;      // Type of exception to catch
            std::string variableName;       // Variable name to bind exception to
            size_t handlerOffset;           // Offset where catch handler starts
        };

        struct ExceptionContext {
            size_t tryBeginOffset;                  // Offset where TRY_BEGIN is emitted
            size_t tryEndOffset;                    // Offset where TRY_END will be emitted (to patch)
            std::vector<CatchHandler> catchHandlers; // Catch handlers for this try block
            size_t finallyOffset;                   // Offset where finally block starts (SIZE_MAX if none)
            std::vector<size_t> exitJumps;          // Jumps to end of try/catch/finally (after normal execution)
            std::vector<size_t> returnJumps;        // Jumps from return statements (need to return after finally)
            std::vector<size_t> breakJumps;         // Jumps from break statements (need to break loop after finally)
            std::vector<size_t> continueJumps;      // Jumps from continue statements (need to continue loop after finally)
            bool hasFinally;                        // True if this try block has a finally block
            bool inFinally;                         // True if currently compiling the finally block
            size_t returnValueSlot;                 // Local slot used to save return value before finally (SIZE_MAX if none)
            size_t hasReturnFlagSlot;               // Local slot for flag indicating if we're on return path (SIZE_MAX if none)
        };

        ExceptionContextManager() = default;
        ~ExceptionContextManager() = default;

        // Exception context management
        void enterTry(size_t tryBeginOffset, bool hasFinally = false);
        void exitTry();
        ExceptionContext& currentContext();
        bool isInTry() const;

        // Register catch handler
        void registerCatchHandler(const std::string& exceptionType,
                                  const std::string& variableName,
                                  size_t handlerOffset);

        // Set finally block offset
        void setFinallyOffset(size_t finallyOffset);

        // Register exit jump (to skip remaining handlers after successful execution)
        void registerExitJump(size_t jumpOffset);

        // Register return jump (from return statement - needs to return after finally)
        void registerReturnJump(size_t jumpOffset);

        // Register break jump (from break statement - needs to break loop after finally)
        void registerBreakJump(size_t jumpOffset);

        // Register continue jump (from continue statement - needs to continue loop after finally)
        void registerContinueJump(size_t jumpOffset);

        // Set try end offset for patching
        void setTryEndOffset(size_t offset);

        // Get current context information
        size_t getTryBeginOffset() const;
        size_t getTryEndOffset() const;
        size_t getFinallyOffset() const;
        const std::vector<CatchHandler>& getCatchHandlers() const;
        const std::vector<size_t>& getExitJumps() const;
        const std::vector<size_t>& getReturnJumps() const;
        const std::vector<size_t>& getBreakJumps() const;
        const std::vector<size_t>& getContinueJumps() const;

        // Set/get return value slot for finally blocks
        void setReturnValueSlot(size_t slot);
        size_t getReturnValueSlot() const;

        // Check if currently in a try block with a finally
        bool hasPendingFinally() const;

        // Mark that we're entering/exiting the finally block
        void enterFinally();
        void exitFinally();
        bool isInFinally() const;

        // Check if there's an outer (parent) try block with a finally
        bool hasOuterFinally() const;

        // Register a return jump with the outer (parent) context
        void registerReturnJumpWithOuter(size_t jumpOffset);

        // Set/get return value slot for the outer (parent) context
        void setReturnValueSlotForOuter(size_t slot);
        size_t getReturnValueSlotForOuter() const;

        // Set/get has return flag slot
        void setHasReturnFlagSlot(size_t slot);
        size_t getHasReturnFlagSlot() const;

        // Get nesting level (0 = outermost try, 1 = first nested try, etc.)
        uint32_t getNestingLevel() const;

    private:
        std::vector<ExceptionContext> contextStack;
    };
}
