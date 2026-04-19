#include "ToUpperCaseFunction.hpp"
#include "../../../errors/ArgumentException.hpp"
#include "../../../errors/RuntimeException.hpp"
#include <algorithm>
#include <cctype>

namespace environment::registry::builtin
{
    Value ToUpperCaseFunction::execute(const std::vector<Value>& args)
    {
        if (args.size() != 1)
        {
            throw errors::ArgumentException("toUpperCase expects exactly 1 argument");
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
            throw errors::RuntimeException("toUpperCase can only be called on strings");
        }

        std::transform(str.begin(), str.end(), str.begin(),
            [](unsigned char c) { return std::toupper(c); });

        return str;
    }

    std::string ToUpperCaseFunction::getName() const
    {
        return "toUpperCase";
    }
}
