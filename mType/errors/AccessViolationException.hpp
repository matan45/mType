#pragma once

#include "TypeException.hpp"
#include "../ast/AccessModifier.hpp"
#include <string>

namespace errors
{
    /**
     * @brief Exception thrown when access control rules are violated
     *
     * This exception is thrown when attempting to access a private, protected,
     * or public member from an inappropriate context.
     */
    class AccessViolationException : public TypeException
    {
    private:
        std::string memberName;
        std::string memberType;        // "field", "method", "constructor"
        ast::AccessModifier accessLevel;
        std::string targetClassName;
        std::string callingContext;

    public:
        /**
         * @brief Construct an access violation exception with a simple message
         * @param msg The error message
         * @param loc Source location of the violation
         */
        explicit AccessViolationException(
            const std::string& msg,
            const SourceLocation& loc = SourceLocation())
            : TypeException(msg, loc),
              memberName(""),
              memberType(""),
              accessLevel(ast::AccessModifier::PRIVATE),
              targetClassName(""),
              callingContext("")
        {
        }

        /**
         * @brief Construct an access violation exception
         * @param member The name of the member being accessed
         * @param type The type of member ("field", "method", "constructor")
         * @param modifier The access modifier that was violated
         * @param targetClass The class containing the member
         * @param callingFrom The context attempting access (class name or "global scope")
         * @param loc Source location of the violation
         */
        explicit AccessViolationException(
            const std::string& member,
            const std::string& type,
            ast::AccessModifier modifier,
            const std::string& targetClass,
            const std::string& callingFrom,
            const SourceLocation& loc);

        // Getters for detailed error information
        const std::string& getMemberName() const { return memberName; }
        const std::string& getMemberType() const { return memberType; }
        ast::AccessModifier getAccessLevel() const { return accessLevel; }
        const std::string& getTargetClassName() const { return targetClassName; }
        const std::string& getCallingContext() const { return callingContext; }

        /**
         * @brief Format a detailed error message
         * @return Formatted error message with suggestion
         */
        std::string formatDetailedMessage() const;
    };
}
