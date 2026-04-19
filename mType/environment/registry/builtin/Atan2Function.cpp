#include "Atan2Function.hpp"
#include "../../../errors/ArgumentException.hpp"
#include "../../../errors/TypeException.hpp"
#include <cmath>

namespace environment::registry::builtin
{
    static double extractNumeric(const Value& arg, const char* op)
    {
        if (isFloat(arg)) return asFloat(arg);
        if (isInt(arg)) return static_cast<double>(asInt(arg));
        throw errors::TypeException(std::string(op) + " expects numeric arguments");
    }

    Value Atan2Function::execute(const std::vector<Value>& args)
    {
        if (args.size() != 2)
        {
            throw errors::ArgumentException("atan2 expects exactly 2 arguments (y, x)");
        }

        const double y = extractNumeric(args[0], "atan2");
        const double x = extractNumeric(args[1], "atan2");
        return std::atan2(y, x);
    }

    std::string Atan2Function::getName() const
    {
        return "atan2";
    }
}
