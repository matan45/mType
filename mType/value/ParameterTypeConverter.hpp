#pragma once
#include "ParameterType.hpp"
#include "ValueType.hpp"
#include <vector>
#include <utility>
#include <string>

namespace value {
    /**
     * @brief Utility class for converting between ParameterType and ValueType representations
     *
     * Separates conversion logic from ParameterType struct following Single Responsibility Principle.
     * This allows ParameterType to focus on representing parameter types while this utility
     * handles format conversions for backward compatibility.
     *
     * Design Principles:
     * - Single Responsibility: Only handles conversions
     * - Stateless: All methods are static (no instance state)
     * - Backward Compatibility: Supports migration from ValueType to ParameterType
     */
    class ParameterTypeConverter {
    public:
        /**
         * @brief Convert from legacy ValueType vector to new ParameterType vector
         * @param oldParams Vector of (name, ValueType) pairs
         * @return Vector of (name, ParameterType) pairs
         *
         * Used when migrating old code or interfacing with legacy APIs
         */
        static std::vector<std::pair<std::string, ParameterType>> fromValueTypeVector(
            const std::vector<std::pair<std::string, ValueType>>& oldParams)
        {
            std::vector<std::pair<std::string, ParameterType>> newParams;
            newParams.reserve(oldParams.size());
            for (const auto& param : oldParams) {
                newParams.emplace_back(param.first, ParameterType(param.second));
            }
            return newParams;
        }

        /**
         * @brief Convert from new ParameterType vector to legacy ValueType vector
         * @param newParams Vector of (name, ParameterType) pairs
         * @return Vector of (name, ValueType) pairs
         *
         * Used when interfacing with legacy code that expects ValueType
         * Note: Loses interface/class information during conversion
         */
        static std::vector<std::pair<std::string, ValueType>> toValueTypeVector(
            const std::vector<std::pair<std::string, ParameterType>>& newParams)
        {
            std::vector<std::pair<std::string, ValueType>> oldParams;
            oldParams.reserve(newParams.size());
            for (const auto& param : newParams) {
                oldParams.emplace_back(param.first, param.second.basicType);
            }
            return oldParams;
        }

        /**
         * @brief Convert single ParameterType to ValueType (lossy conversion)
         * @param param ParameterType to convert
         * @return Basic ValueType (loses interface/class information)
         */
        static ValueType toValueType(const ParameterType& param) {
            return param.basicType;
        }

        /**
         * @brief Convert single ValueType to ParameterType
         * @param type ValueType to convert
         * @return ParameterType with only basic type information
         */
        static ParameterType fromValueType(ValueType type) {
            return ParameterType(type);
        }
    };
}
