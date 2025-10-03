#include "AccessContext.hpp"
#include "../../runtimeTypes/klass/ClassDefinition.hpp"
#include "../../runtimeTypes/klass/ObjectInstance.hpp"

namespace evaluator::base
{
    AccessContext AccessContext::forInstanceAccess(
        std::shared_ptr<runtimeTypes::klass::ObjectInstance> callingInstance,
        std::shared_ptr<runtimeTypes::klass::ClassDefinition> targetClassDef,
        const SourceLocation& loc)
    {
        AccessContext context;
        context.targetClass = targetClassDef;
        context.targetClassName = targetClassDef ? targetClassDef->getName() : "";
        context.isStaticAccess = false;
        context.location = loc;

        if (callingInstance)
        {
            context.callingClass = callingInstance->getClassDefinition();
            context.callingClassName = context.callingClass ? context.callingClass->getName() : "";

            // Check if same class
            context.isSameClass = (context.callingClassName == context.targetClassName);

            // Check if subclass
            if (!context.isSameClass && context.callingClass && targetClassDef)
            {
                context.isSubclass = context.callingClass->isSubclassOf(context.targetClassName);
            }
        }
        else
        {
            // No calling instance - global access
            context.callingClassName = "";
            context.callingClass = nullptr;
            context.isSameClass = false;
            context.isSubclass = false;
        }

        return context;
    }

    AccessContext AccessContext::forStaticAccess(
        const std::string& callingClass,
        std::shared_ptr<runtimeTypes::klass::ClassDefinition> targetClassDef,
        const SourceLocation& loc)
    {
        AccessContext context;
        context.callingClassName = callingClass;
        context.targetClass = targetClassDef;
        context.targetClassName = targetClassDef ? targetClassDef->getName() : "";
        context.isStaticAccess = true;
        context.location = loc;

        // Check if same class
        context.isSameClass = (context.callingClassName == context.targetClassName);

        // Note: For static access, we don't have the calling ClassDefinition object
        // so we can't check subclass relationship easily. This would require
        // looking up the calling class from the registry.
        context.isSubclass = false;

        return context;
    }

    AccessContext AccessContext::forGlobalAccess(
        std::shared_ptr<runtimeTypes::klass::ClassDefinition> targetClassDef,
        const SourceLocation& loc)
    {
        AccessContext context;
        context.callingClassName = "";
        context.callingClass = nullptr;
        context.targetClass = targetClassDef;
        context.targetClassName = targetClassDef ? targetClassDef->getName() : "";
        context.isStaticAccess = false;
        context.isSameClass = false;
        context.isSubclass = false;
        context.location = loc;

        return context;
    }
}
