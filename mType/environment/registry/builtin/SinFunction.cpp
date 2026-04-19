#include "SinFunction.hpp"
#include "../../../errors/ArgumentException.hpp"
#include "../../../errors/TypeException.hpp"
#include <cmath>

namespace environment::registry::builtin
{
    Value SinFunction::execute(const std::vector<Value>& args)
    {
        if (args.size() != 1)
        {
            throw errors::ArgumentException("sin expects exactly 1 argument");
        }

        const Value& arg = args[0];
        if (isFloat(arg))
        {
            return std::sin(asFloat(arg));
        }
        if (isInt(arg))
        {
            return std::sin(static_cast<double>(asInt(arg)));
        }
        throw errors::TypeException("sin expects a numeric argument");
    }

    std::string SinFunction::getName() const
    {
        return "sin";
    }
}
