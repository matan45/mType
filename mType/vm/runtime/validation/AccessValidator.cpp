#include "AccessValidator.hpp"

namespace vm::runtime::validation
{
    void AccessValidator::validateFieldAccess(
        const std::string& fieldName,
        ast::AccessModifier accessModifier,
        const AccessContext& context
    )
    {
        if (!isAccessAllowed(accessModifier, context))
        {
            std::string callingContext = context.callingClassName.empty() ?
                "global scope" : context.callingClassName;

            throw errors::AccessViolationException(
                fieldName,
                "field",
                accessModifier,
                context.targetClassName,
                callingContext,
                context.location
            );
        }
    }

    void AccessValidator::validateMethodAccess(
        const std::string& methodName,
        ast::AccessModifier accessModifier,
        const AccessContext& context
    )
    {
        if (!isAccessAllowed(accessModifier, context))
        {
            std::string callingContext = context.callingClassName.empty() ?
                "global scope" : context.callingClassName;

            throw errors::AccessViolationException(
                methodName,
                "method",
                accessModifier,
                context.targetClassName,
                callingContext,
                context.location
            );
        }
    }

    void AccessValidator::validateConstructorAccess(
        const std::string& className,
        ast::AccessModifier accessModifier,
        const AccessContext& context
    )
    {
        if (!isAccessAllowed(accessModifier, context))
        {
            std::string callingContext = context.callingClassName.empty() ?
                "global scope" : context.callingClassName;

            throw errors::AccessViolationException(
                className + " constructor",
                "constructor",
                accessModifier,
                context.targetClassName,
                callingContext,
                context.location
            );
        }
    }

    bool AccessValidator::canAccess(
        ast::AccessModifier accessModifier,
        const AccessContext& context
    )
    {
        return isAccessAllowed(accessModifier, context);
    }

    bool AccessValidator::isAccessAllowed(
        ast::AccessModifier modifier,
        const AccessContext& context
    )
    {
        // Fast path: PUBLIC is always accessible
        if (modifier == ast::AccessModifier::PUBLIC)
        {
            return true;
        }

        // PRIVATE: Only accessible from same class
        if (modifier == ast::AccessModifier::PRIVATE)
        {
            return context.isSameClass;
        }

        // PROTECTED: Accessible from same class or subclasses
        if (modifier == ast::AccessModifier::PROTECTED)
        {
            return context.isSameClass || context.isSubclass;
        }

        // Default: deny access
        return false;
    }
}
