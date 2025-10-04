#include "AccessValidator.hpp"
#include "../../errors/AccessViolationException.hpp"

namespace evaluator::validation
{
    void AccessValidator::validateFieldAccess(
        const AccessContext& context,
        const runtimeTypes::klass::FieldDefinition& field)
    {
        if (!isAccessAllowed(field.getAccessModifier(), context))
        {
            std::string callingContext = context.callingClassName.empty() ?
                "global scope" : context.callingClassName;

            throw errors::AccessViolationException(
                field.getName(),
                "field",
                field.getAccessModifier(),
                context.targetClassName,
                callingContext,
                context.location);
        }
    }

    void AccessValidator::validateMethodAccess(
        const AccessContext& context,
        const runtimeTypes::klass::MethodDefinition& method)
    {
        if (!isAccessAllowed(method.getAccessModifier(), context))
        {
            std::string callingContext = context.callingClassName.empty() ?
                "global scope" : context.callingClassName;

            throw errors::AccessViolationException(
                method.getName(),
                "method",
                method.getAccessModifier(),
                context.targetClassName,
                callingContext,
                context.location);
        }
    }

    void AccessValidator::validateConstructorAccess(
        const AccessContext& context,
        const runtimeTypes::klass::ConstructorDefinition& constructor)
    {
        if (!isAccessAllowed(constructor.getAccessModifier(), context))
        {
            std::string callingContext = context.callingClassName.empty() ?
                "global scope" : context.callingClassName;

            throw errors::AccessViolationException(
                context.targetClassName + " constructor",
                "constructor",
                constructor.getAccessModifier(),
                context.targetClassName,
                callingContext,
                context.location);
        }
    }

    bool AccessValidator::canAccess(
        const AccessContext& context,
        AccessModifier memberModifier)
    {
        return isAccessAllowed(memberModifier, context);
    }

    bool AccessValidator::isAccessAllowed(
        AccessModifier modifier,
        const AccessContext& context)
    {
        // Fast path: PUBLIC is always accessible
        if (modifier == AccessModifier::PUBLIC)
        {
            return true;
        }

        // PRIVATE: Only accessible from same class
        if (modifier == AccessModifier::PRIVATE)
        {
            return context.isSameClass;
        }

        // PROTECTED: Accessible from same class or subclasses
        if (modifier == AccessModifier::PROTECTED)
        {
            return context.isSameClass || context.isSubclass;
        }

        // Default: deny access
        return false;
    }
}
