#include "ConstructorDefinition.hpp"
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

            // Use type name if it's an object, otherwise use basic type
            if (paramType.basicType == value::ValueType::OBJECT && paramType.className.has_value()) {
                signature += paramType.className.value();
            } else {
                signature += ::types::TypeConversionUtils::getTypeDisplayName(paramType.basicType);
            }
        }
        return signature;
    }

    bool ConstructorDefinition::matchesArgCount(size_t argCount) const
    {
        return parametersWithTypes.size() == argCount;
    }
}