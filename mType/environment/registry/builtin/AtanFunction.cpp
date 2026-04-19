#include "AtanFunction.hpp"
#include "../../../errors/ArgumentException.hpp"
#include "../../../errors/TypeException.hpp"
#include <cmath>

namespace environment::registry::builtin
{
    Value AtanFunction::execute(const std::vector<Value>& args)
    {
        if (args.size() != 1)
        {
            throw errors::ArgumentException("atan expects exactly 1 argument");
        }

        const Value& arg = args[0];
        if (isFloat(arg))
        {
            return std::atan(asFloat(arg));
        }
        if (isInt(arg))
        {
            return std::atan(static_cast<double>(asInt(arg)));
        }
        throw errors::TypeException("atan expects a numeric argument");
    }

    std::string AtanFunction::getName() const
    {
        return "atan";
    }
}
