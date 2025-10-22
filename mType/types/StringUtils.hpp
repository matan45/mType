#pragma once

#include <string>

namespace types
{
    /**
     * @brief String utility functions for type parsing and manipulation
     */
    namespace StringUtils
    {
        /**
         * @brief Trim leading and trailing whitespace from a string
         * @param str The string to trim
         * @return Trimmed string with leading/trailing whitespace removed
         */
        std::string trimWhitespace(const std::string& str);

        /**
         * @brief Check if a string is empty or contains only whitespace
         * @param str The string to check
         * @return true if string is empty or all whitespace
         */
        bool isWhitespace(const std::string& str);
    }
}
