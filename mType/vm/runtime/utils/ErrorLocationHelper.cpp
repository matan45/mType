#include "ErrorLocationHelper.hpp"
#include "../../../errors/UserException.hpp"
#include "../../../errors/RuntimeException.hpp"
#include "../../../value/ObjectInstancePool.hpp"
#include "../../../runtimeTypes/klass/ObjectInstance.hpp"
#include "../../../runtimeTypes/klass/ClassDefinition.hpp"
#include "../../../environment/Environment.hpp"
#include "../../../environment/registry/ClassRegistry.hpp"

namespace vm::runtime::utils
{
    namespace
    {
        // Walk the user-level "typeName -> RuntimeException -> Exception"
        // fallback chain. Returns the first ClassDefinition that's actually
        // loaded in the registry, or nullptr if none are.
        std::shared_ptr<runtimeTypes::klass::ClassDefinition>
        resolveExceptionClass(const std::shared_ptr<environment::Environment>& env,
                              const std::string& typeName)
        {
            if (!env) return nullptr;
            auto registry = env->getClassRegistry();
            if (!registry) return nullptr;

            if (auto cd = registry->findClass(typeName)) return cd;
            if (typeName != "RuntimeException")
            {
                if (auto cd = registry->findClass("RuntimeException")) return cd;
            }
            if (typeName != "Exception")
            {
                if (auto cd = registry->findClass("Exception")) return cd;
            }
            return nullptr;
        }

        // Construct a user Exception-subclass instance, populate `message`,
        // and throw it as UserException with the given source location.
        // Caller has already resolved `classDef` via resolveExceptionClass.
        [[noreturn]] void throwAsUserException(
            const std::shared_ptr<runtimeTypes::klass::ClassDefinition>& classDef,
            const std::string& message,
            const errors::SourceLocation& srcLoc)
        {
            auto instance = value::ObjectInstancePool::getInstance().acquire(classDef);
            instance->setField("message", message);
            instance->setField("stackTrace", std::string{});
            throw errors::UserException(value::Value(instance), classDef->getName(), srcLoc);
        }
    }

    void ErrorLocationHelper::throwUserException(
        const ExecutionContext& context,
        const std::shared_ptr<environment::Environment>& env,
        const std::string& typeName,
        const std::string& message)
    {
        auto classDef = resolveExceptionClass(env, typeName);
        if (!classDef)
        {
            // Nothing in lib/exceptions/ is loaded — preserve the old
            // abort behaviour by falling through to a bare RuntimeException
            // with source location attached.
            throwRuntimeError(context, message);
        }

        errors::SourceLocation srcLoc;
        if (auto* loc = context.program->getSourceLocation(context.instructionPointer))
        {
            srcLoc = errors::SourceLocation(loc->filename, loc->line, loc->column);
        }
        throwAsUserException(classDef, message, srcLoc);
    }

    void ErrorLocationHelper::throwUserException(
        const std::shared_ptr<environment::Environment>& env,
        const std::string& typeName,
        const std::string& message)
    {
        auto classDef = resolveExceptionClass(env, typeName);
        if (!classDef)
        {
            throw errors::RuntimeException(message);
        }
        throwAsUserException(classDef, message, errors::SourceLocation{});
    }
}
