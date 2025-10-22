#pragma once

#include "DependencyType.hpp"
#include <string>

namespace circularDependency
{
    /**
     * @brief Utility functions for DependencyType operations
     */
    namespace DependencyTypeUtils
    {
        /**
         * @brief Converts a DependencyType enum value to its string representation
         * @param type The dependency type to convert
         * @return String representation of the dependency type
         */
        std::string toString(DependencyType type);
    }
}
