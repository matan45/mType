#include "HashCodeFunction.hpp"
#include "../../../errors/ArgumentException.hpp"
#include "../../../runtimeTypes/klass/ObjectInstance.hpp"
#include "../../../value/ValueShim.hpp"
#include <functional>

namespace environment::registry::builtin
{
    Value HashCodeFunction::execute(const std::vector<Value>& args)
    {
        if (args.size() != 1)
        {
            throw errors::ArgumentException("hashCode expects exactly 1 argument");
        }

        const Value& arg = args[0];

        if (isString(arg))
        {
            std::hash<std::string> hasher;
            return static_cast<int64_t>(hasher(asString(arg)) & 0x7FFFFFFF);
        }
        if (isInternedString(arg))
        {
            std::hash<std::string> hasher;
            return static_cast<int64_t>(hasher(asInternedString(arg).getString()) & 0x7FFFFFFF);
        }
        if (isInt(arg))
        {
            std::hash<int64_t> hasher;
            return static_cast<int64_t>(hasher(asInt(arg)) & 0x7FFFFFFF);
        }
        if (isFloat(arg))
        {
            std::hash<double> hasher;
            return static_cast<int64_t>(hasher(asFloat(arg)) & 0x7FFFFFFF);
        }
        if (isBool(arg))
        {
            return asBool(arg) ? static_cast<int64_t>(1231) : static_cast<int64_t>(1237);
        }
        if (isObject(arg))
        {
            auto obj = asObject(arg);
            if (!obj)
            {
                return static_cast<int64_t>(0);
            }
            std::string contentHash = obj->getContentHash();
            std::hash<std::string> hasher;
            return static_cast<int64_t>(hasher(contentHash) & 0x7FFFFFFF);
        }
        if (isNullType(arg))
        {
            return static_cast<int64_t>(0);
        }
        return static_cast<int64_t>(0);
    }

    std::string HashCodeFunction::getName() const
    {
        return "hashCode";
    }
}
