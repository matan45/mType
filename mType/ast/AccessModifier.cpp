#include "AccessModifier.hpp"
#include <stdexcept>

namespace ast
{
    const char* accessModifierToString(AccessModifier modifier)
    {
        switch (modifier)
        {
        case AccessModifier::PRIVATE:
            return "private";
        case AccessModifier::PUBLIC:
            return "public";
        case AccessModifier::PROTECTED:
            return "protected";
        default:
            return "unknown";
        }
    }

    AccessModifier stringToAccessModifier(const std::string& str)
    {
        if (str == "private")
        {
            return AccessModifier::PRIVATE;
        }
        if (str == "public")
        {
            return AccessModifier::PUBLIC;
        }
        if (str == "protected")
        {
            return AccessModifier::PROTECTED;
        }
        throw std::invalid_argument("Invalid access modifier: " + str);
    }

    AccessModifier getDefaultAccessModifier(bool isInterface)
    {
        return isInterface ? AccessModifier::PUBLIC : AccessModifier::PRIVATE;
    }
}
