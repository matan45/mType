#include "ConstructorDefinition.hpp"
#include <cstddef>
#include "../../types/TypeConversionUtils.hpp"

namespace runtimeTypes::klass
{
    const std::vector<std::pair<std::string, ValueType>>& ConstructorDefinition::getParameters() const
    {
        // Lazy computation - only compute when needed
        if (!parametersCacheValid) {
            cachedParameters.clear();
            cachedParameters.reserve(parametersWithTypes.size());
            for (const auto& [name, paramType] : parametersWithTypes) {
                cachedParameters.emplace_back(name, paramType.basicType);
            }
            parametersCacheValid = true;
        }
        return cachedParameters;
    }

    std::string ConstructorDefinition::getTypeSignature() const
    {
        if (parametersWithTypes.empty()) {
            return "";
        }

        std::string signature;
        for (size_t i = 0; i < parametersWithTypes.size(); ++i) {
            if (i > 0) signature += ",";

            const auto& paramType = parametersWithTypes[i].second;

            // MYT-282: ParameterType overload — same precise-array story as
            // MethodDefinition::generateSignature.
            signature += ::types::TypeConversionUtils::getTypeDisplayName(paramType);
        }
        return signature;
    }

    bool ConstructorDefinition::matchesArgCount(size_t argCount) const
    {
        return parametersWithTypes.size() == argCount;
    }
}