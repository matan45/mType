#include "ConstructorDefinition.hpp"

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

    bool ConstructorDefinition::matchesArgCount(size_t argCount) const
    {
        return parametersWithTypes.size() == argCount;
    }
}