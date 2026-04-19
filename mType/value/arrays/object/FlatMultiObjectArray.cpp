#include "FlatMultiObjectArray.hpp"
#include "../../../runtimeTypes/klass/FieldDefinition.hpp"
#include "../../ValueShim.hpp"

namespace mType
{
    namespace value
    {
        namespace arrays
        {
            FlatMultiObjectArray::FlatMultiObjectArray(
                std::shared_ptr<runtimeTypes::klass::ClassDefinition> classDef,
                const std::vector<size_t>& dims
            ) : ObjectArrayBase(classDef),
                dimensions_(dims),
                parent_(nullptr),
                viewOffset_(0)
            {
                if (dims.empty())
                {
                    throw std::invalid_argument("Dimensions cannot be empty");
                }

                calculateStrides();
                totalSize_ = calculateTotalSize();
                initializeFieldArrays(totalSize_);
            }

            // Private constructor for creating views
            FlatMultiObjectArray::FlatMultiObjectArray(
                std::shared_ptr<FlatMultiObjectArray> parentArray,
                size_t offset,
                const std::vector<size_t>& dims
            ) : ObjectArrayBase(parentArray->classDefinition_),
                dimensions_(dims),
                parent_(parentArray),
                viewOffset_(offset)
            {
                calculateStrides();
                totalSize_ = calculateTotalSize();
                // No field array initialization - this is a view into parent
            }

            void FlatMultiObjectArray::calculateStrides()
            {
                strides_.resize(dimensions_.size());
                if (dimensions_.empty()) return;

                strides_.back() = 1; // Last dimension has stride 1
                for (int i = static_cast<int>(dimensions_.size()) - 2; i >= 0; --i)
                {
                    strides_[i] = strides_[i + 1] * dimensions_[i + 1];
                }
            }

            size_t FlatMultiObjectArray::calculateTotalSize() const
            {
                if (dimensions_.empty()) return 0;
                return std::accumulate(dimensions_.begin(), dimensions_.end(),
                                       size_t(1), std::multiplies<size_t>());
            }

            size_t FlatMultiObjectArray::calculateLinearIndex(const std::vector<size_t>& indices) const
            {
                if (indices.size() != dimensions_.size())
                {
                    return SIZE_MAX; // Invalid
                }

                size_t linearIndex = 0;
                for (size_t i = 0; i < indices.size(); ++i)
                {
                    if (indices[i] >= dimensions_[i])
                    {
                        return SIZE_MAX; // Out of bounds
                    }
                    linearIndex += indices[i] * strides_[i];
                }
                return linearIndex;
            }

            // Element access

            ::value::Value FlatMultiObjectArray::get(const std::vector<size_t>& indices) const
            {
                size_t linearIndex = calculateLinearIndex(indices);
                if (linearIndex == SIZE_MAX || linearIndex >= totalSize_)
                {
                    return std::monostate{}; // Out of bounds
                }

                size_t effectiveIndex = getEffectiveOffset() + linearIndex;
                return materializeInstance(effectiveIndex);
            }

            void FlatMultiObjectArray::set(const std::vector<size_t>& indices, const ::value::Value& value)
            {
                size_t linearIndex = calculateLinearIndex(indices);
                if (linearIndex == SIZE_MAX)
                {
                    throw std::out_of_range("Invalid multi-dimensional array indices");
                }
                if (linearIndex >= totalSize_)
                {
                    throw std::out_of_range("Calculated index exceeds array bounds");
                }

                if (!::value::isObject(value))
                {
                    throw std::runtime_error("Value must be an ObjectInstance");
                }

                auto instance = ::value::asObject(value);

                if (instance->getClassDefinition()->getClassName() != classDefinition_->getClassName())
                {
                    throw std::runtime_error("ObjectInstance class mismatch");
                }

                size_t effectiveIndex = getEffectiveOffset() + linearIndex;
                decomposeInstance(effectiveIndex, instance);
            }

            ::value::Value FlatMultiObjectArray::getLinear(size_t linearIndex) const
            {
                if (linearIndex >= totalSize_)
                {
                    throw std::out_of_range("Linear index out of bounds");
                }

                size_t effectiveIndex = getEffectiveOffset() + linearIndex;
                return materializeInstance(effectiveIndex);
            }

            void FlatMultiObjectArray::setLinear(size_t linearIndex, const ::value::Value& value)
            {
                if (linearIndex >= totalSize_)
                {
                    throw std::out_of_range("Linear index out of bounds");
                }

                if (!::value::isObject(value))
                {
                    throw std::runtime_error("Value must be an ObjectInstance");
                }

                auto instance = ::value::asObject(value);

                if (instance->getClassDefinition()->getClassName() != classDefinition_->getClassName())
                {
                    throw std::runtime_error("ObjectInstance class mismatch");
                }

                size_t effectiveIndex = getEffectiveOffset() + linearIndex;
                decomposeInstance(effectiveIndex, instance);
            }

            // Field-level access

            ::value::Value FlatMultiObjectArray::getField(
                const std::vector<size_t>& indices,
                const std::string& fieldName
            ) const
            {
                size_t linearIndex = calculateLinearIndex(indices);
                if (linearIndex == SIZE_MAX || linearIndex >= totalSize_)
                {
                    throw std::out_of_range("Invalid indices");
                }

                const auto& storage = getFieldArraysStorage();
                auto it = storage.find(fieldName);
                if (it == storage.end())
                {
                    throw std::runtime_error("Field not found: " + fieldName);
                }

                size_t effectiveIndex = getEffectiveOffset() + linearIndex;
                return it->second->get(effectiveIndex);
            }

            void FlatMultiObjectArray::setField(
                const std::vector<size_t>& indices,
                const std::string& fieldName,
                const ::value::Value& value
            )
            {
                size_t linearIndex = calculateLinearIndex(indices);
                if (linearIndex == SIZE_MAX || linearIndex >= totalSize_)
                {
                    throw std::out_of_range("Invalid indices");
                }

                auto& storage = getFieldArraysStorage();
                auto it = storage.find(fieldName);
                if (it == storage.end())
                {
                    throw std::runtime_error("Field not found: " + fieldName);
                }

                if (!validateFieldType(fieldName, value))
                {
                    throw std::runtime_error("Field type mismatch for: " + fieldName);
                }

                size_t effectiveIndex = getEffectiveOffset() + linearIndex;
                it->second->set(effectiveIndex, value);
            }

            // Sub-array slicing

            std::shared_ptr<FlatMultiObjectArray> FlatMultiObjectArray::getSubArray(size_t index)
            {
                if (dimensions_.empty() || index >= dimensions_[0])
                {
                    return nullptr;
                }

                // Create sub-dimensions (remove first dimension)
                std::vector<size_t> subDims(dimensions_.begin() + 1, dimensions_.end());
                if (subDims.empty())
                {
                    return nullptr; // Would be scalar
                }

                // Calculate offset into parent's data for this sub-array
                size_t subArrayOffset = getEffectiveOffset() + (index * strides_[0]);

                // Get the root parent (if this is already a view, use the same root parent)
                std::shared_ptr<FlatMultiObjectArray> rootParent = isView() ? parent_ : shared_from_this();

                // Create a VIEW into the parent array (not a copy!)
                // This allows modifications to propagate to the parent
                auto subArray = std::shared_ptr<FlatMultiObjectArray>(
                    new FlatMultiObjectArray(rootParent, subArrayOffset, subDims)
                );

                return subArray;
            }

            // Memory statistics

            FlatMultiObjectArray::MultiDimMemoryStats FlatMultiObjectArray::getMemoryStats() const
            {
                MultiDimMemoryStats stats;

                const auto& storage = getFieldArraysStorage();

                stats.objectCount = totalSize_;
                stats.fieldCount = storage.size();
                stats.dimensionCount = dimensions_.size();

                // Calculate SoA memory
                stats.soaMemoryUsage = sizeof(FlatMultiObjectArray);
                stats.soaMemoryUsage += dimensions_.capacity() * sizeof(size_t);
                stats.soaMemoryUsage += strides_.capacity() * sizeof(size_t);

                for (const auto& [fieldName, fieldArray] : storage)
                {
                    stats.soaMemoryUsage += fieldName.capacity();
                    stats.soaMemoryUsage += fieldArray->capacity() * 8; // Rough estimate
                    stats.soaMemoryUsage += 64; // Hash map node overhead
                }

                // Estimate AoS memory
                stats.aosMemoryUsage = totalSize_ * (
                    sizeof(runtimeTypes::klass::ObjectInstance) +
                    64 + // shared_ptr overhead
                    storage.size() * (64 + 32) // hash map per object
                );

                stats.memorySaved = stats.aosMemoryUsage > stats.soaMemoryUsage
                                        ? stats.aosMemoryUsage - stats.soaMemoryUsage
                                        : 0;

                stats.savingsPercentage = stats.aosMemoryUsage > 0
                                              ? (static_cast<double>(stats.memorySaved) / stats.aosMemoryUsage) * 100.0
                                              : 0.0;

                return stats;
            }

        } // namespace arrays
    } // namespace value
} // namespace mType
