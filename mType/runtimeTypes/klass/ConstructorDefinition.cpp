#include "ConstructorDefinition.hpp"

namespace runtimeTypes::klass
{
    bool ConstructorDefinition::matchesArgCount(size_t argCount) const
    {
        return parameters.size() == argCount;
    }
}