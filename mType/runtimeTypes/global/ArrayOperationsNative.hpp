#pragma once
#include <memory>
#include <string>
#include <vector>
#include "../../value/ValueType.hpp"
#include "../../environment/Environment.hpp"

namespace runtimeTypes::global
{
    /**
     * @brief Native bridge for high-performance array operations
     */
    class ArrayOperationsNative
    {
    public:
        /**
         * Register all array operation functions in the environment
         */
        static void registerAll(std::shared_ptr<environment::Environment> env);
    };
}
