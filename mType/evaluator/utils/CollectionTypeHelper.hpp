#pragma once

#include "../../value/ValueType.hpp"
#include "../../runtimeTypes/collections/Array.hpp"
#include "../../runtimeTypes/collections/Map.hpp"
#include "../../runtimeTypes/collections/Set.hpp"
#include "../../runtimeTypes/collections/Stack.hpp"
#include "../../runtimeTypes/collections/Queue.hpp"
#include <memory>
#include <string>

namespace evaluator::utils
{
    using namespace value;
    using namespace runtimeTypes::collections;

    /**
     * @brief Helper class for collection type checking and identification
     * Replaces long if-chains with clean, centralized type checking
     */
    class CollectionTypeHelper
    {
    public:
        /**
         * @brief Check if a Value contains any collection type
         * @param value The value to check
         * @return true if value is a collection, false otherwise
         */
        static bool isCollection(const Value& value)
        {
            return isCollectionType<Array>(value) ||
                   isCollectionType<Map>(value) ||
                   isCollectionType<Set>(value) ||
                   isCollectionType<Stack>(value) ||
                   isCollectionType<Queue>(value);
        }

        /**
         * @brief Get the type name of a collection for error messages
         * @param value The collection value
         * @return String type name (e.g., "Array", "Map", "Set", "Stack", "Queue")
         */
        static std::string getCollectionTypeName(const Value& value)
        {
            if (isCollectionType<Array>(value)) return "Array";
            if (isCollectionType<Map>(value)) return "Map";
            if (isCollectionType<Set>(value)) return "Set";
            if (isCollectionType<Stack>(value)) return "Stack";
            if (isCollectionType<Queue>(value)) return "Queue";
            return "Unknown";
        }

        /**
         * @brief Check if a Value contains a specific collection type
         * @tparam CollectionType The collection type to check for
         * @param value The value to check
         * @return true if value is of the specified collection type
         */
        template<typename CollectionType>
        static bool isCollectionType(const Value& value)
        {
            return std::holds_alternative<std::shared_ptr<CollectionType>>(value);
        }

        /**
         * @brief Extract collection of specific type from Value
         * @tparam CollectionType The collection type to extract
         * @param value The value containing the collection
         * @return Shared pointer to the collection
         * @throws std::bad_variant_access if value doesn't contain the expected type
         */
        template<typename CollectionType>
        static std::shared_ptr<CollectionType> extractCollection(const Value& value)
        {
            return std::get<std::shared_ptr<CollectionType>>(value);
        }

        /**
         * @brief Check if value is Array type
         */
        static bool isArray(const Value& value) { return isCollectionType<Array>(value); }

        /**
         * @brief Check if value is Map type
         */
        static bool isMap(const Value& value) { return isCollectionType<Map>(value); }

        /**
         * @brief Check if value is Set type
         */
        static bool isSet(const Value& value) { return isCollectionType<Set>(value); }

        /**
         * @brief Check if value is Stack type
         */
        static bool isStack(const Value& value) { return isCollectionType<Stack>(value); }

        /**
         * @brief Check if value is Queue type
         */
        static bool isQueue(const Value& value) { return isCollectionType<Queue>(value); }
    };
}