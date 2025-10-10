#pragma once
#include <vector>
#include <string>
#include <cstddef>

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
        };

        ExceptionContextManager() = default;
        ~ExceptionContextManager() = default;

        // Exception context management
        void enterTry(size_t tryBeginOffset);
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

        // Set try end offset for patching
        void setTryEndOffset(size_t offset);

        // Get current context information
        size_t getTryBeginOffset() const;
        size_t getTryEndOffset() const;
        size_t getFinallyOffset() const;
        const std::vector<CatchHandler>& getCatchHandlers() const;
        const std::vector<size_t>& getExitJumps() const;

    private:
        std::vector<ExceptionContext> contextStack;
    };
}
