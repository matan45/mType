#include "StringUtils.hpp"

namespace types
{
    namespace StringUtils
    {
        std::string trimWhitespace(const std::string& str)
        {
            if (str.empty())
            {
                return str;
            }

            // Find first non-whitespace character
            size_t start = str.find_first_not_of(" \t\n\r");
            if (start == std::string::npos)
            {
                return ""; // String is all whitespace
            }

            // Find last non-whitespace character
            size_t end = str.find_last_not_of(" \t\n\r");

            return str.substr(start, end - start + 1);
        }

        bool isWhitespace(const std::string& str)
        {
            if (str.empty())
            {
                return true;
            }

            return str.find_first_not_of(" \t\n\r") == std::string::npos;
        }
    }
}
