#pragma once
#include "ParseException.hpp"
#include <string>

namespace errors
{
    class DuplicateDeclarationException : public ParseException
    {
    private:
        std::string duplicateName;
        std::string declarationType; // "class", "interface", or "function"
        SourceLocation firstDeclaration;
        SourceLocation secondDeclaration;

        static std::string formatMessage(
            const std::string& type,
            const std::string& name,
            const SourceLocation& first,
            const SourceLocation& second)
        {
            std::string article = (type == "interface") ? "an" : "a";
            return "Duplicate " + type + " declaration: '" + name +
                   "' has already been declared as " + article + " " + type +
                   " at " + first.toString();
        }

    public:
        explicit DuplicateDeclarationException(
            const std::string& type,
            const std::string& name,
            const SourceLocation& first,
            const SourceLocation& second)
            : ParseException(formatMessage(type, name, first, second), second),
              duplicateName(name),
              declarationType(type),
              firstDeclaration(first),
              secondDeclaration(second)
        {
        }

        // Getters for additional context
        const std::string& getDuplicateName() const { return duplicateName; }
        const std::string& getDeclarationType() const { return declarationType; }
        const SourceLocation& getFirstDeclaration() const { return firstDeclaration; }
        const SourceLocation& getSecondDeclaration() const { return secondDeclaration; }
    };
}
