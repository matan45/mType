#pragma once
#include "../../errors/SourceLocation.hpp"
#include "../TokenStream.hpp"
#include <string>
#include <vector>
#include <memory>

namespace parser::error
{
    using namespace errors;

    enum class ErrorSeverity
    {
        Warning,
        Error,
        Fatal
    };

    enum class RecoveryStrategy
    {
        SkipToNext,
        SyncToStatement,
        SyncToBlock,
        Abort
    };

    struct ErrorContext
    {
        SourceLocation location;
        std::string message;
        ErrorSeverity severity;
        std::string parserContext;
        std::vector<std::string> suggestions;

        ErrorContext(const SourceLocation& loc, const std::string& msg,
                    ErrorSeverity sev = ErrorSeverity::Error,
                    const std::string& context = "")
            : location(loc), message(msg), severity(sev), parserContext(context) {}
    };

    class ErrorHandler
    {
    private:
        std::vector<ErrorContext> errors;
        bool hasFatalError = false;

    public:
        void reportError(const ErrorContext& context);
        void reportWarning(const std::string& message, const SourceLocation& location,
                          const std::string& context = "");

        bool hasErrors() const noexcept { return !errors.empty(); }
        bool hasFatal() const noexcept { return hasFatalError; }
        const std::vector<ErrorContext>& getErrors() const noexcept { return errors; }

        void clear() noexcept;

        RecoveryStrategy getRecoveryStrategy(const std::string& parserContext) const;
    };
}