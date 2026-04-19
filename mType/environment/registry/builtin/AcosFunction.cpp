#include "AcosFunction.hpp"
#include "../../../errors/ArgumentException.hpp"
#include "../../../errors/TypeException.hpp"
#include <cmath>

namespace environment::registry::builtin
{
    Value AcosFunction::execute(const std::vector<Value>& args)
    {
        if (args.size() != 1)
        {
            throw errors::ArgumentException("acos expects exactly 1 argument");
        }

        const Value& arg = args[0];
        if (isFloat(arg))
        {
            return std::acos(asFloat(arg));
        }
        if (isInt(arg))
        {
            return std::acos(static_cast<double>(asInt(arg)));
        }
        throw errors::TypeException("acos expects a numeric argument");
    }

    std::string AcosFunction::getName() const
    {
        return "acos";
    }
}
