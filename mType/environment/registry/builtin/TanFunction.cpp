#include "TanFunction.hpp"
#include "../../../errors/ArgumentException.hpp"
#include "../../../errors/TypeException.hpp"
#include <cmath>

namespace environment::registry::builtin
{
    Value TanFunction::execute(const std::vector<Value>& args)
    {
        if (args.size() != 1)
        {
            throw errors::ArgumentException("tan expects exactly 1 argument");
        }

        const Value& arg = args[0];
        if (isFloat(arg))
        {
            return std::tan(asFloat(arg));
        }
        if (isInt(arg))
        {
            return std::tan(static_cast<double>(asInt(arg)));
        }
        throw errors::TypeException("tan expects a numeric argument");
    }

    std::string TanFunction::getName() const
    {
        return "tan";
    }
}
