#include "SubstringFunction.hpp"
#include "../../../errors/ArgumentException.hpp"
#include "../../../errors/RuntimeException.hpp"

namespace environment::registry::builtin
{
    static std::string extractString(const Value& arg, const char* msg)
    {
        if (isString(arg)) return asString(arg);
        if (isInternedString(arg)) return asInternedString(arg).getString();
        throw errors::RuntimeException(msg);
    }

    static int64_t extractInt(const Value& arg, const char* msg)
    {
        if (isInt(arg)) return asInt(arg);
        throw errors::RuntimeException(msg);
    }

    Value SubstringFunction::execute(const std::vector<Value>& args)
    {
        if (args.size() != 3)
        {
            throw errors::ArgumentException("substring expects exactly 3 arguments (string, startIndex, length)");
        }

        const std::string str = extractString(args[0], "substring: first argument must be a string");
        const int64_t startIndex = extractInt(args[1], "substring: second argument (startIndex) must be an integer");
        const int64_t length = extractInt(args[2], "substring: third argument (length) must be an integer");

        if (startIndex < 0)
        {
            throw errors::RuntimeException("substring: startIndex cannot be negative");
        }

        if (length < 0)
        {
            throw errors::RuntimeException("substring: length cannot be negative");
        }

        size_t strLen = str.length();
        if (static_cast<size_t>(startIndex) > strLen)
        {
            throw errors::RuntimeException("substring: startIndex out of bounds");
        }

        size_t actualLength = static_cast<size_t>(length);
        if (static_cast<size_t>(startIndex) + actualLength > strLen)
        {
            actualLength = strLen - static_cast<size_t>(startIndex);
        }

        return str.substr(static_cast<size_t>(startIndex), actualLength);
    }

    std::string SubstringFunction::getName() const
    {
        return "substring";
    }
}
