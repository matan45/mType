#pragma once
#include "TypeException.hpp"
#include <cstddef>
#include <string>
#include <vector>

namespace errors
{
    /**
     * Exception thrown when a method or function call matches multiple overloads equally well
     */
    class AmbiguousCallException : public TypeException
    {
    private:
        std::string methodName;
        std::vector<std::string> candidateSignatures;
        std::vector<std::string> argumentTypes;

        static std::string formatMessage(
            const std::string& name,
            const std::vector<std::string>& candidates,
            const std::vector<std::string>& argTypes)
        {
            std::string message = "ambiguous call to '" + name + "' with arguments (";

            // Format argument types
            for (size_t i = 0; i < argTypes.size(); ++i) {
                if (i > 0) message += ", ";
                message += argTypes[i];
            }
            message += ").\n\nMultiple overloads match equally well:\n";

            // List all candidate signatures
            for (size_t i = 0; i < candidates.size(); ++i) {
                message += "  " + std::to_string(i + 1) + ". " + name + "(" + candidates[i] + ")\n";
            }

            message += "\nPlease provide explicit type conversion to disambiguate the call.";
            return message;
        }

    public:
        explicit AmbiguousCallException(
            const std::string& name,
            const std::vector<std::string>& candidates,
            const std::vector<std::string>& argTypes,
            const SourceLocation& loc = SourceLocation())
            : TypeException(formatMessage(name, candidates, argTypes), loc),
              methodName(name),
              candidateSignatures(candidates),
              argumentTypes(argTypes)
        {
        }

        // Getters for additional context
        const std::string& getMethodName() const { return methodName; }
        const std::vector<std::string>& getCandidateSignatures() const { return candidateSignatures; }
        const std::vector<std::string>& getArgumentTypes() const { return argumentTypes; }
    };
}
