#include "AsinFunction.hpp"
#include "../../../errors/ArgumentException.hpp"
#include "../../../errors/TypeException.hpp"
#include <cmath>

namespace environment::registry::builtin
{
    Value AsinFunction::execute(const std::vector<Value>& args)
    {
        if (args.size() != 1)
        {
            throw errors::ArgumentException("asin expects exactly 1 argument");
        }

        const Value& arg = args[0];
        if (isFloat(arg))
        {
            return std::asin(asFloat(arg));
        }
        if (isInt(arg))
        {
            return std::asin(static_cast<double>(asInt(arg)));
        }
        throw errors::TypeException("asin expects a numeric argument");
    }

    std::string AsinFunction::getName() const
    {
        return "asin";
    }
}
