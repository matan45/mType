#include "SqrtFunction.hpp"
#include "../../../errors/ArgumentException.hpp"
#include "../../../errors/TypeException.hpp"
#include <cmath>

namespace environment::registry::builtin
{
    Value SqrtFunction::execute(const std::vector<Value>& args)
    {
        if (args.size() != 1)
        {
            throw errors::ArgumentException("sqrt expects exactly 1 argument");
        }

        const Value& arg = args[0];
        if (isFloat(arg))
        {
            return std::sqrt(asFloat(arg));
        }
        if (isInt(arg))
        {
            return std::sqrt(static_cast<double>(asInt(arg)));
        }
        throw errors::TypeException("sqrt expects a numeric argument");
    }

    std::string SqrtFunction::getName() const
    {
        return "sqrt";
    }
}
