#pragma once
#include "ParseException.hpp"
#include <string>
#include <vector>

namespace errors
{
    /**
     * Exception thrown when a method or function is declared with a signature
     * that already exists
     */
    class DuplicateSignatureException : public ParseException
    {
    private:
        std::string methodName;
        std::string signature;
        std::string declarationType; // "method", "static method", "function", or "constructor"
        SourceLocation firstDeclaration;
        SourceLocation secondDeclaration;

        static std::string formatMessage(
            const std::string& type,
            const std::string& name,
            const std::string& sig,
            const SourceLocation& first,
            const SourceLocation& second)
        {
            std::string fullName = name + "(" + sig + ")";
            return "Duplicate " + type + " signature: '" + fullName +
                   "' has already been declared at " + first.toString();
        }

    public:
        explicit DuplicateSignatureException(
            const std::string& type,
            const std::string& name,
            const std::string& sig,
            const SourceLocation& first,
            const SourceLocation& second)
            : ParseException(formatMessage(type, name, sig, first, second), second),
              methodName(name),
              signature(sig),
              declarationType(type),
              firstDeclaration(first),
              secondDeclaration(second)
        {
        }

        // Getters for additional context
        const std::string& getMethodName() const { return methodName; }
        const std::string& getSignature() const { return signature; }
        const std::string& getDeclarationType() const { return declarationType; }
        const SourceLocation& getFirstDeclaration() const { return firstDeclaration; }
        const SourceLocation& getSecondDeclaration() const { return secondDeclaration; }
    };
}
