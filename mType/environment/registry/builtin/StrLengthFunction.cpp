#include "StrLengthFunction.hpp"
#include "../../../errors/ArgumentException.hpp"
#include "../../../errors/RuntimeException.hpp"

namespace environment::registry::builtin
{
    Value StrLengthFunction::execute(const std::vector<Value>& args)
    {
        if (args.size() != 1)
        {
            throw errors::ArgumentException("str::length expects exactly 1 argument");
        }

        const Value& arg = args[0];
        if (isString(arg))
        {
            return static_cast<int>(asString(arg).length());
        }
        if (isInternedString(arg))
        {
            return static_cast<int>(asInternedString(arg).getString().length());
        }
        throw errors::RuntimeException("str::length can only be called on strings");
    }

    std::string StrLengthFunction::getName() const
    {
        return "strLength";
    }
}
