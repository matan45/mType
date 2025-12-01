#include "ExpectedTypeContext.hpp"
#include "../../../types/TypeSubstitutionService.hpp"

namespace vm::compiler::types
{
    std::vector<std::string> ExpectedTypeContext::extractGenericArguments() const
    {
        if (!isActive || expectedClassName.empty())
        {
            return {};
        }

        // Use TypeSubstitutionService to parse the generic arguments
        ::types::TypeSubstitutionService service;
        return service.extractTypeArguments(expectedClassName);
    }

    std::string ExpectedTypeContext::getBaseClassName() const
    {
        if (!isActive || expectedClassName.empty())
        {
            return "";
        }

        // Use TypeSubstitutionService to extract base type name
        ::types::TypeSubstitutionService service;
        return service.extractBaseTypeName(expectedClassName);
    }

    bool ExpectedTypeContext::hasGenericArguments() const
    {
        if (!isActive || expectedClassName.empty())
        {
            return false;
        }

        return expectedClassName.find('<') != std::string::npos;
    }
}
