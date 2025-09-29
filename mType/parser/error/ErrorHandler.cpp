#include "ErrorHandler.hpp"
#include <iostream>

namespace parser::error
{
    void ErrorHandler::reportError(const ErrorContext& context)
    {
        errors.push_back(context);

        if (context.severity == ErrorSeverity::Fatal)
        {
            hasFatalError = true;
        }

        std::cerr << "[" << (context.severity == ErrorSeverity::Warning ? "WARNING" : "ERROR") << "] "
                  << "Line " << context.location.getLine() << ", Column " << context.location.getColumn()
                  << ": " << context.message;

        if (!context.parserContext.empty())
        {
            std::cerr << " (in " << context.parserContext << ")";
        }

        std::cerr << std::endl;

        if (!context.suggestions.empty())
        {
            std::cerr << "  Suggestions:" << std::endl;
            for (const auto& suggestion : context.suggestions)
            {
                std::cerr << "    - " << suggestion << std::endl;
            }
        }
    }

    void ErrorHandler::reportWarning(const std::string& message, const SourceLocation& location,
                                    const std::string& context)
    {
        ErrorContext warning(location, message, ErrorSeverity::Warning, context);
        reportError(warning);
    }

    void ErrorHandler::clear() noexcept
    {
        errors.clear();
        hasFatalError = false;
    }

    RecoveryStrategy ErrorHandler::getRecoveryStrategy(const std::string& parserContext) const
    {
        if (parserContext == "StatementParser" || parserContext == "ControlFlowParser")
        {
            return RecoveryStrategy::SyncToStatement;
        }
        else if (parserContext == "ClassParser" || parserContext == "MethodParser")
        {
            return RecoveryStrategy::SyncToBlock;
        }
        else if (parserContext == "ExpressionParser")
        {
            return RecoveryStrategy::SkipToNext;
        }

        return RecoveryStrategy::SyncToStatement;
    }
}