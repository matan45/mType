#pragma once

#include <stdexcept>
#include <string>

namespace packagemanager
{
    inline bool isValidPackageName(const std::string& name)
    {
        if (name.empty())
        {
            return false;
        }

        for (char c : name)
        {
            const bool isAlpha = (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z');
            const bool isDigit = c >= '0' && c <= '9';
            const bool isAllowedSymbol = c == '_' || c == '-';

            if (!isAlpha && !isDigit && !isAllowedSymbol)
            {
                return false;
            }
        }

        return true;
    }

    inline void validatePackageName(const std::string& name,
                                    const std::string& context = "package name")
    {
        if (!isValidPackageName(name))
        {
            throw std::runtime_error("Invalid " + context + ": " + name);
        }
    }
}
