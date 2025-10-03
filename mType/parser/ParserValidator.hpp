#pragma once
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

        // NEW: Helper function to validate parent class naming convention
        static bool isValidParentClassName(std::string_view name);
    };
}

