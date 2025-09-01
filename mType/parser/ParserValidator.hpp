#pragma once
#include <string>
#include <string_view>

namespace parser
{
    class ParserValidator
    {
    public:
        // Helper function to validate class naming convention
        static bool isValidClassName(std::string_view name);

        // Helper function to validate namespace naming convention
        static bool isValidNamespaceName(std::string_view name);
    };
}

