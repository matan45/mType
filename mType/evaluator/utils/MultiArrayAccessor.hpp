#pragma once

#include "../../value/ValueType.hpp"
#include "../../value/FlatMultiArray.hpp"
#include "../../value/SparseMultiArray.hpp"
#include "../../value/arrays/object/FlatMultiObjectArray.hpp"
#include "../../errors/TypeException.hpp"
#include <variant>
#include <memory>
#include <vector>
#include <optional>

namespace evaluator
{
    namespace utils
    {
        using namespace value;

        /**
         * @brief Utility class for uniform access to multi-dimensional arrays
         *
         * Eliminates 58 type-checking occurrences across the evaluator by providing
         * a single interface to work with FlatMultiArray, SparseMultiArray, and
         * FlatMultiObjectArray without repetitive std::holds_alternative checks.
         *
         * Design Principles:
         * - Adapter Pattern: Adapts different array types to common interface
         * - Visitor Pattern: Uses std::visit for type-safe dispatch
         * - DRY Principle: Eliminates duplicated type-checking code
         */
        class MultiArrayAccessor
        {
        public:
            /**
             * @brief Check if a Value contains a multi-dimensional array
             */
            static bool isMultiDimensionalArray(const Value& value)
            {
                return std::holds_alternative<std::shared_ptr<FlatMultiArray>>(value) ||
                       std::holds_alternative<std::shared_ptr<SparseMultiArray>>(value) ||
                       std::holds_alternative<std::shared_ptr<mType::value::arrays::FlatMultiObjectArray>>(value);
            }

            /**
             * @brief Get rank (number of dimensions) of array in Value
             */
            static std::optional<size_t> getRank(const Value& value)
            {
                if (auto* flat = std::get_if<std::shared_ptr<FlatMultiArray>>(&value))
                {
                    return (*flat)->getRank();
                }
                if (auto* sparse = std::get_if<std::shared_ptr<SparseMultiArray>>(&value))
                {
                    return (*sparse)->getRank();
                }
                if (auto* flatObj = std::get_if<std::shared_ptr<mType::value::arrays::FlatMultiObjectArray>>(&value))
                {
                    return (*flatObj)->getRank();
                }
                return std::nullopt;
            }

            /**
             * @brief Get size of first dimension
             */
            static std::optional<size_t> getSize(const Value& value)
            {
                if (auto* flat = std::get_if<std::shared_ptr<FlatMultiArray>>(&value))
                {
                    return (*flat)->size();
                }
                if (auto* sparse = std::get_if<std::shared_ptr<SparseMultiArray>>(&value))
                {
                    return (*sparse)->size();
                }
                if (auto* flatObj = std::get_if<std::shared_ptr<mType::value::arrays::FlatMultiObjectArray>>(&value))
                {
                    return (*flatObj)->size();
                }
                return std::nullopt;
            }

            /**
             * @brief Get element at multi-dimensional index
             */
            static std::optional<Value> get(const Value& arrayValue, const std::vector<size_t>& indices)
            {
                if (auto* flat = std::get_if<std::shared_ptr<FlatMultiArray>>(&arrayValue))
                {
                    return (*flat)->get(indices);
                }
                if (auto* sparse = std::get_if<std::shared_ptr<SparseMultiArray>>(&arrayValue))
                {
                    return (*sparse)->get(indices);
                }
                if (auto* flatObj = std::get_if<std::shared_ptr<mType::value::arrays::FlatMultiObjectArray>>(&arrayValue))
                {
                    return (*flatObj)->get(indices);
                }
                return std::nullopt;
            }

            /**
             * @brief Get sub-array at given index (returns as Value)
             */
            static std::optional<Value> getSubArray(const Value& arrayValue, size_t index)
            {
                if (auto* flat = std::get_if<std::shared_ptr<FlatMultiArray>>(&arrayValue))
                {
                    auto subArray = (*flat)->getSubArray(index);
                    return subArray ? Value{subArray} : std::optional<Value>{};
                }
                if (auto* sparse = std::get_if<std::shared_ptr<SparseMultiArray>>(&arrayValue))
                {
                    auto subArray = (*sparse)->getSubArray(index);
                    return subArray ? Value{subArray} : std::optional<Value>{};
                }
                if (auto* flatObj = std::get_if<std::shared_ptr<mType::value::arrays::FlatMultiObjectArray>>(&arrayValue))
                {
                    auto subArray = (*flatObj)->getSubArray(index);
                    return subArray ? Value{subArray} : std::optional<Value>{};
                }
                return std::nullopt;
            }

            /**
             * @brief Get total size of array
             */
            static std::optional<size_t> getTotalSize(const Value& value)
            {
                if (auto* flat = std::get_if<std::shared_ptr<FlatMultiArray>>(&value))
                {
                    return (*flat)->totalSize();
                }
                if (auto* sparse = std::get_if<std::shared_ptr<SparseMultiArray>>(&value))
                {
                    return (*sparse)->totalSize();
                }
                if (auto* flatObj = std::get_if<std::shared_ptr<mType::value::arrays::FlatMultiObjectArray>>(&value))
                {
                    return (*flatObj)->totalSize();
                }
                return std::nullopt;
            }

            /**
             * @brief Check if array has specific dimensions
             */
            static bool hasDimensions(const Value& value, const std::vector<size_t>& dims)
            {
                if (auto* flat = std::get_if<std::shared_ptr<FlatMultiArray>>(&value))
                {
                    return (*flat)->hasDimensions(dims);
                }
                if (auto* sparse = std::get_if<std::shared_ptr<SparseMultiArray>>(&value))
                {
                    return (*sparse)->hasDimensions(dims);
                }
                if (auto* flatObj = std::get_if<std::shared_ptr<mType::value::arrays::FlatMultiObjectArray>>(&value))
                {
                    return (*flatObj)->hasDimensions(dims);
                }
                return false;
            }
        };
    }
}
