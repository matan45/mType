#include "ExpectedTypeContext.hpp"
#include "GenericTypeResolver.hpp"

namespace vm::compiler::types
{
    std::vector<std::string> ExpectedTypeContext::extractGenericArguments() const
    {
        if (!isActive || expectedClassName.empty())
        {
            return {};
        }

        // Use GenericTypeResolver to parse the generic arguments
        GenericTypeResolver resolver;
        return resolver.extractTypeArguments(expectedClassName);
    }

    std::string ExpectedTypeContext::getBaseClassName() const
    {
        if (!isActive || expectedClassName.empty())
        {
            return "";
        }

        // Use GenericTypeResolver to extract base type name
        GenericTypeResolver resolver;
        return resolver.extractBaseTypeName(expectedClassName);
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
