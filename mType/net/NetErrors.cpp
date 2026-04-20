#include "NetErrors.hpp"
#include "../runtimeTypes/klass/ObjectInstance.hpp"
#include "../runtimeTypes/klass/ClassDefinition.hpp"
#include "../value/ObjectInstancePool.hpp"
#include "../errors/UserException.hpp"
#include "../errors/RuntimeException.hpp"

namespace net
{
    namespace
    {
        // Split "kind:rest of message" — returns ("kind", "rest") or ("", message).
        std::pair<std::string, std::string> splitKind(const std::string& msg)
        {
            size_t colon = msg.find(':');
            if (colon == std::string::npos) return {"", msg};
            std::string kind = msg.substr(0, colon);
            std::string rest = msg.substr(colon + 1);
            if (kind == "dns" || kind == "timeout" || kind == "connection" || kind == "http")
            {
                return {kind, rest};
            }
            return {"", msg};
        }

        std::string kindToClassName(const std::string& kind)
        {
            if (kind == "dns") return "DnsException";
            if (kind == "timeout") return "TimeoutException";
            if (kind == "connection") return "ConnectionException";
            if (kind == "http") return "HttpException";
            return "NetworkException";
        }
    }

    [[noreturn]] void throwNetError(std::shared_ptr<environment::Environment> env,
                                    const std::string& message)
    {
        auto [kind, rest] = splitKind(message);
        std::string className = kindToClassName(kind);
        throwTypedNetError(env, className, rest, 0);
    }

    [[noreturn]] void throwTypedNetError(std::shared_ptr<environment::Environment> env,
                                         const std::string& className,
                                         const std::string& message,
                                         int statusCode)
    {
        if (!env)
        {
            throw errors::RuntimeException("net error (no env): " + message);
        }
        auto classDef = env->findClass(className);
        if (!classDef)
        {
            // Fall back to plain RuntimeException if the mType-side class hasn't
            // been loaded yet (e.g. the lib isn't imported in this script).
            throw errors::RuntimeException(className + ": " + message);
        }

        auto instance = value::ObjectInstancePool::getInstance().acquire(classDef);
        instance->setField("message", message);
        if (statusCode != 0)
        {
            instance->setField("status", static_cast<int64_t>(statusCode));
        }

        throw errors::UserException(value::Value(instance), className);
    }
}
