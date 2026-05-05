#pragma once
#include "TypeException.hpp"
#include <cstddef>
#include <string>
#include <vector>

namespace errors
{
    /**
     * Exception thrown when no overload matches the provided arguments
     */
    class NoMatchingOverloadException : public TypeException
    {
    private:
        std::string methodName;
        std::vector<std::string> availableSignatures;
        std::vector<std::string> argumentTypes;

        static std::string formatMessage(
            const std::string& name,
            const std::vector<std::string>& available,
            const std::vector<std::string>& argTypes)
        {
            std::string message = "No matching overload for call to '" + name + "' with arguments (";

            // Format argument types
            for (size_t i = 0; i < argTypes.size(); ++i) {
                if (i > 0) message += ", ";
                message += argTypes[i];
            }
            message += ").";

            // List available overloads
            if (!available.empty()) {
                message += "\n\nAvailable overloads:\n";
                for (size_t i = 0; i < available.size(); ++i) {
                    message += "  " + std::to_string(i + 1) + ". " + name + "(" + available[i] + ")\n";
                }
            } else {
                message += "\n\nNo overloads of '" + name + "' are defined.";
            }

            return message;
        }

    public:
        explicit NoMatchingOverloadException(
            const std::string& name,
            const std::vector<std::string>& available,
            const std::vector<std::string>& argTypes,
            const SourceLocation& loc = SourceLocation())
            : TypeException(formatMessage(name, available, argTypes), loc),
              methodName(name),
              availableSignatures(available),
              argumentTypes(argTypes)
        {
        }

        // Getters for additional context
        const std::string& getMethodName() const { return methodName; }
        const std::vector<std::string>& getAvailableSignatures() const { return availableSignatures; }
        const std::vector<std::string>& getArgumentTypes() const { return argumentTypes; }
    };
}
