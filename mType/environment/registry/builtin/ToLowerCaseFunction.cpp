#include "ToLowerCaseFunction.hpp"
#include "../../../errors/ArgumentException.hpp"
#include "../../../errors/RuntimeException.hpp"
#include <algorithm>
#include <cctype>

namespace environment::registry::builtin
{
    Value ToLowerCaseFunction::execute(const std::vector<Value>& args)
    {
        if (args.size() != 1)
        {
            throw errors::ArgumentException("toLowerCase expects exactly 1 argument");
        }

        std::string str;
        const Value& arg = args[0];
        if (isString(arg))
        {
            str = asString(arg);
        }
        else if (isInternedString(arg))
        {
            str = asInternedString(arg).getString();
        }
        else
        {
            throw errors::RuntimeException("toLowerCase can only be called on strings");
        }

        std::transform(str.begin(), str.end(), str.begin(),
            [](unsigned char c) { return std::tolower(c); });

        return str;
    }

    std::string ToLowerCaseFunction::getName() const
    {
        return "toLowerCase";
    }
}
