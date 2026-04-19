#include "CosFunction.hpp"
#include "../../../errors/ArgumentException.hpp"
#include "../../../errors/TypeException.hpp"
#include <cmath>

namespace environment::registry::builtin
{
    Value CosFunction::execute(const std::vector<Value>& args)
    {
        if (args.size() != 1)
        {
            throw errors::ArgumentException("cos expects exactly 1 argument");
        }

        const Value& arg = args[0];
        if (isFloat(arg))
        {
            return std::cos(asFloat(arg));
        }
        if (isInt(arg))
        {
            return std::cos(static_cast<double>(asInt(arg)));
        }
        throw errors::TypeException("cos expects a numeric argument");
    }

    std::string CosFunction::getName() const
    {
        return "cos";
    }
}
