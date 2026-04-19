#include "ParsePrimitiveFunction.hpp"
#include "../../../errors/ArgumentException.hpp"

namespace environment::registry::builtin
{
    Value ParsePrimitiveFunction::execute(const std::vector<Value>& args)
    {
        if (args.size() != 1)
        {
            throw errors::ArgumentException("parsePrimitive expects exactly 1 argument");
        }

        const Value& arg = args[0];
        if (isString(arg))
        {
            return asString(arg);
        }
        if (isInternedString(arg))
        {
            return asInternedString(arg).getString();
        }
        if (isInt(arg))
        {
            return std::to_string(asInt(arg));
        }
        if (isFloat(arg))
        {
            std::string str = std::to_string(asFloat(arg));
            str.erase(str.find_last_not_of('0') + 1, std::string::npos);
            if (str.back() == '.')
            {
                str += '0';
            }
            return str;
        }
        if (isBool(arg))
        {
            return asBool(arg) ? std::string("true") : std::string("false");
        }
        if (isVoid(arg))
        {
            return std::string("void");
        }
        if (isNullType(arg))
        {
            return std::string("null");
        }
        return std::string("unknown");
    }

    std::string ParsePrimitiveFunction::getName() const
    {
        return "parsePrimitive";
    }
}
