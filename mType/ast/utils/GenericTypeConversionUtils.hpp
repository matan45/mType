#pragma once
#include "../GenericType.hpp"
#include "../../value/ValueType.hpp"
#include <vector>
#include <string>
#include <memory>
#include <utility>

namespace ast::utils
{
    /**
     * Utility functions for converting between ValueType and GenericType.
     * Used for backward compatibility in legacy constructors and getters.
     */
    class GenericTypeConversionUtils
    {
    public:
        /**
         * Converts a ValueType to GenericType.
         * @param type The ValueType to convert
         * @return A shared_ptr to the new GenericType
         */
        static std::shared_ptr<GenericType> convertValueTypeToGenericType(value::ValueType type);

        /**
         * Converts a GenericType to ValueType.
         * @param genericType The GenericType to convert (must not be null)
         * @return The corresponding ValueType (OBJECT for generic parameters)
         * @throws std::invalid_argument if genericType is null
         *
         * Note: Generic type parameters (T, E, K, V) return ValueType::OBJECT
         *       since they represent any reference type at runtime.
         */
        static value::ValueType convertGenericTypeToValueType(const std::shared_ptr<GenericType>& genericType);

        /**
         * Converts a parameter list from ValueType to GenericType.
         * @param params Parameter list with ValueType
         * @return Parameter list with shared_ptr<GenericType>
         */
        static std::vector<std::pair<std::string, std::shared_ptr<GenericType>>>
            convertParametersToGenericType(const std::vector<std::pair<std::string, value::ValueType>>& params);

        /**
         * Converts a parameter list from GenericType to ValueType.
         * @param params Parameter list with shared_ptr<GenericType> (none should be null)
         * @return Parameter list with ValueType
         * @throws std::invalid_argument if any GenericType in params is null
         */
        static std::vector<std::pair<std::string, value::ValueType>>
            convertParametersToValueType(const std::vector<std::pair<std::string, std::shared_ptr<GenericType>>>& params);

        /**
         * Clones a GenericType.
         * @param genericType The GenericType to clone (can be nullptr)
         * @return A cloned shared_ptr<GenericType> or nullptr
         */
        static std::shared_ptr<GenericType> cloneGenericType(const std::shared_ptr<GenericType>& genericType);

        /**
         * Clones a parameter list with GenericType.
         * @param params The parameter list to clone (none should be null)
         * @return A cloned parameter list with deep-copied GenericType instances
         * @throws std::invalid_argument if any GenericType in params is null
         */
        static std::vector<std::pair<std::string, std::shared_ptr<GenericType>>>
            cloneGenericParameters(const std::vector<std::pair<std::string, std::shared_ptr<GenericType>>>& params);
    };
}
