#include "VisibilityModifier.hpp"

namespace ast
{
    const char* visibilityModifierToString(VisibilityModifier modifier)
    {
        switch (modifier)
        {
        case VisibilityModifier::PUBLIC:
            return "public";
        case VisibilityModifier::PRIVATE:
            return "private";
        default:
            return "unknown";
        }
    }

    VisibilityModifier stringToVisibilityModifier(const std::string& str)
    {
        if (str == "public") return VisibilityModifier::PUBLIC;
        if (str == "private") return VisibilityModifier::PRIVATE;
        throw std::invalid_argument("Invalid visibility modifier: " + str);
    }

    VisibilityModifier getDefaultVisibilityModifier()
    {
        return VisibilityModifier::PUBLIC;
    }
}
