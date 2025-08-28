#pragma once
#include <string>

namespace parser
{
    class ParserValidator
    {
    public:
        // Helper function to validate class naming convention
        static bool isValidClassName(const std::string& name);

        // Helper function to validate namespace naming convention
        static bool isValidNamespaceName(const std::string& name);
    };
}

