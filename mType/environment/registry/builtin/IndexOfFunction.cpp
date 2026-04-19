#include "IndexOfFunction.hpp"
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

    Value IndexOfFunction::execute(const std::vector<Value>& args)
    {
        if (args.size() != 2)
        {
            throw errors::ArgumentException("indexOf expects exactly 2 arguments (haystack, needle)");
        }

        const std::string haystack = extractString(args[0], "indexOf: first argument (haystack) must be a string");
        const std::string needle = extractString(args[1], "indexOf: second argument (needle) must be a string");

        size_t pos = haystack.find(needle);
        if (pos == std::string::npos)
        {
            return -1;
        }

        return static_cast<int>(pos);
    }

    std::string IndexOfFunction::getName() const
    {
        return "indexOf";
    }
}
