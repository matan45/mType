#include "FlatMultiObjectArray.hpp"
#include "../../../runtimeTypes/klass/FieldDefinition.hpp"

namespace mType {
namespace value {
namespace arrays {

FlatMultiObjectArray::FlatMultiObjectArray(
    std::shared_ptr<runtimeTypes::klass::ClassDefinition> classDef,
    const std::vector<size_t>& dims
) : classDefinition_(classDef),
    dimensions_(dims)
{
    if (!classDef) {
        throw std::invalid_argument("ClassDefinition cannot be null");
    }

    if (dims.empty()) {
        throw std::invalid_argument("Dimensions cannot be empty");
    }

    calculateStrides();
    totalSize_ = calculateTotalSize();
    initializeFieldArrays();
}

void FlatMultiObjectArray::calculateStrides() {
    strides_.resize(dimensions_.size());
    if (dimensions_.empty()) return;

    strides_.back() = 1; // Last dimension has stride 1
    for (int i = static_cast<int>(dimensions_.size()) - 2; i >= 0; --i) {
        strides_[i] = strides_[i + 1] * dimensions_[i + 1];
    }
}

size_t FlatMultiObjectArray::calculateTotalSize() const {
    if (dimensions_.empty()) return 0;
    return std::accumulate(dimensions_.begin(), dimensions_.end(),
                          size_t(1), std::multiplies<size_t>());
}

void FlatMultiObjectArray::initializeFieldArrays() {
    const auto& fields = classDefinition_->getInstanceFields();

    for (const auto& [fieldName, fieldDef] : fields) {
        auto fieldArray = createFieldArray(fieldDef, totalSize_);
        fieldArrays_[fieldName] = fieldArray;
    }
}

std::shared_ptr<IArray> FlatMultiObjectArray::createFieldArray(
    const std::shared_ptr<runtimeTypes::klass::FieldDefinition>& field,
    size_t capacity
) {
    ::value::ValueType fieldType = field->getType();

    switch (fieldType) {
        case ::value::ValueType::INT:
            return std::make_shared<IntArray>(capacity);
        case ::value::ValueType::FLOAT:
            return std::make_shared<FloatArray>(capacity);
        case ::value::ValueType::BOOL:
            return std::make_shared<BoolArray>(capacity);
        case ::value::ValueType::STRING:
            return std::make_shared<StringArray>(capacity);
        default:
            // Object types - placeholder
            return std::make_shared<IntArray>(capacity);
    }
}

size_t FlatMultiObjectArray::calculateLinearIndex(const std::vector<size_t>& indices) const {
    if (indices.size() != dimensions_.size()) {
        return SIZE_MAX; // Invalid
    }

    size_t linearIndex = 0;
    for (size_t i = 0; i < indices.size(); ++i) {
        if (indices[i] >= dimensions_[i]) {
            return SIZE_MAX; // Out of bounds
        }
        linearIndex += indices[i] * strides_[i];
    }
    return linearIndex;
}

// Element access

::value::Value FlatMultiObjectArray::get(const std::vector<size_t>& indices) const {
    size_t linearIndex = calculateLinearIndex(indices);
    if (linearIndex == SIZE_MAX || linearIndex >= totalSize_) {
        return std::monostate{}; // Out of bounds
    }

    return materializeInstance(linearIndex);
}

void FlatMultiObjectArray::set(const std::vector<size_t>& indices, const ::value::Value& value) {
    size_t linearIndex = calculateLinearIndex(indices);
    if (linearIndex == SIZE_MAX) {
        throw std::out_of_range("Invalid multi-dimensional array indices");
    }
    if (linearIndex >= totalSize_) {
        throw std::out_of_range("Calculated index exceeds array bounds");
    }

    if (!std::holds_alternative<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(value)) {
        throw std::runtime_error("Value must be an ObjectInstance");
    }

    auto instance = std::get<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(value);

    if (instance->getClassDefinition()->getClassName() != classDefinition_->getClassName()) {
        throw std::runtime_error("ObjectInstance class mismatch");
    }

    decomposeInstance(linearIndex, instance);
}

::value::Value FlatMultiObjectArray::getLinear(size_t linearIndex) const {
    if (linearIndex >= totalSize_) {
        throw std::out_of_range("Linear index out of bounds");
    }

    return materializeInstance(linearIndex);
}

void FlatMultiObjectArray::setLinear(size_t linearIndex, const ::value::Value& value) {
    if (linearIndex >= totalSize_) {
        throw std::out_of_range("Linear index out of bounds");
    }

    if (!std::holds_alternative<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(value)) {
        throw std::runtime_error("Value must be an ObjectInstance");
    }

    auto instance = std::get<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(value);

    if (instance->getClassDefinition()->getClassName() != classDefinition_->getClassName()) {
        throw std::runtime_error("ObjectInstance class mismatch");
    }

    decomposeInstance(linearIndex, instance);
}

// Field-level access

::value::Value FlatMultiObjectArray::getField(
    const std::vector<size_t>& indices,
    const std::string& fieldName
) const {
    size_t linearIndex = calculateLinearIndex(indices);
    if (linearIndex == SIZE_MAX || linearIndex >= totalSize_) {
        throw std::out_of_range("Invalid indices");
    }

    auto it = fieldArrays_.find(fieldName);
    if (it == fieldArrays_.end()) {
        throw std::runtime_error("Field not found: " + fieldName);
    }

    return it->second->get(linearIndex);
}

void FlatMultiObjectArray::setField(
    const std::vector<size_t>& indices,
    const std::string& fieldName,
    const ::value::Value& value
) {
    size_t linearIndex = calculateLinearIndex(indices);
    if (linearIndex == SIZE_MAX || linearIndex >= totalSize_) {
        throw std::out_of_range("Invalid indices");
    }

    auto it = fieldArrays_.find(fieldName);
    if (it == fieldArrays_.end()) {
        throw std::runtime_error("Field not found: " + fieldName);
    }

    if (!validateFieldType(fieldName, value)) {
        throw std::runtime_error("Field type mismatch for: " + fieldName);
    }

    it->second->set(linearIndex, value);
}

std::shared_ptr<IArray> FlatMultiObjectArray::getFieldArray(const std::string& fieldName) const {
    auto it = fieldArrays_.find(fieldName);
    if (it == fieldArrays_.end()) {
        return nullptr;
    }
    return it->second;
}

// Sub-array slicing

std::shared_ptr<FlatMultiObjectArray> FlatMultiObjectArray::getSubArray(size_t index) const {
    if (dimensions_.empty() || index >= dimensions_[0]) {
        return nullptr;
    }

    // Create sub-dimensions (remove first dimension)
    std::vector<size_t> subDims(dimensions_.begin() + 1, dimensions_.end());
    if (subDims.empty()) {
        return nullptr; // Would be scalar
    }

    auto subArray = std::make_shared<FlatMultiObjectArray>(classDefinition_, subDims);

    // Copy relevant portion of each field array
    size_t subArraySize = subArray->totalSize();
    size_t startOffset = index * strides_[0];

    for (const auto& [fieldName, fieldArray] : fieldArrays_) {
        auto subFieldArray = subArray->fieldArrays_[fieldName];

        for (size_t i = 0; i < subArraySize; ++i) {
            if (startOffset + i < totalSize_) {
                ::value::Value val = fieldArray->get(startOffset + i);
                subFieldArray->set(i, val);
            }
        }
    }

    return subArray;
}

// Metadata

std::vector<std::string> FlatMultiObjectArray::getFieldNames() const {
    std::vector<std::string> names;
    names.reserve(fieldArrays_.size());

    for (const auto& [fieldName, fieldArray] : fieldArrays_) {
        names.push_back(fieldName);
    }

    return names;
}

bool FlatMultiObjectArray::hasField(const std::string& fieldName) const {
    return fieldArrays_.find(fieldName) != fieldArrays_.end();
}

// Memory statistics

FlatMultiObjectArray::MemoryStats FlatMultiObjectArray::getMemoryStats() const {
    MemoryStats stats;

    stats.objectCount = totalSize_;
    stats.fieldCount = fieldArrays_.size();
    stats.dimensionCount = dimensions_.size();

    // Calculate SoA memory
    stats.soaMemoryUsage = sizeof(FlatMultiObjectArray);
    stats.soaMemoryUsage += dimensions_.capacity() * sizeof(size_t);
    stats.soaMemoryUsage += strides_.capacity() * sizeof(size_t);

    for (const auto& [fieldName, fieldArray] : fieldArrays_) {
        stats.soaMemoryUsage += fieldName.capacity();
        stats.soaMemoryUsage += fieldArray->capacity() * 8; // Rough estimate
        stats.soaMemoryUsage += 64; // Hash map node overhead
    }

    // Estimate AoS memory
    stats.aosMemoryUsage = totalSize_ * (
        sizeof(runtimeTypes::klass::ObjectInstance) +
        64 + // shared_ptr overhead
        fieldArrays_.size() * (64 + 32) // hash map per object
    );

    stats.memorySaved = stats.aosMemoryUsage > stats.soaMemoryUsage
        ? stats.aosMemoryUsage - stats.soaMemoryUsage
        : 0;

    stats.savingsPercentage = stats.aosMemoryUsage > 0
        ? (static_cast<double>(stats.memorySaved) / stats.aosMemoryUsage) * 100.0
        : 0.0;

    return stats;
}

bool FlatMultiObjectArray::supportsSIMD() const {
    for (const auto& [fieldName, fieldArray] : fieldArrays_) {
        if (fieldArray->supportsSIMD()) {
            return true;
        }
    }
    return false;
}

// Private helper methods

std::shared_ptr<runtimeTypes::klass::ObjectInstance> FlatMultiObjectArray::materializeInstance(
    size_t linearIndex
) const {
    auto instance = std::make_shared<runtimeTypes::klass::ObjectInstance>(classDefinition_);

    for (const auto& [fieldName, fieldArray] : fieldArrays_) {
        ::value::Value fieldValue = fieldArray->get(linearIndex);
        instance->setField(fieldName, fieldValue);
    }

    return instance;
}

void FlatMultiObjectArray::decomposeInstance(
    size_t linearIndex,
    const std::shared_ptr<runtimeTypes::klass::ObjectInstance>& instance
) {
    for (const auto& [fieldName, fieldArray] : fieldArrays_) {
        ::value::Value fieldValue = instance->getFieldValue(fieldName);
        fieldArray->set(linearIndex, fieldValue);
    }
}

bool FlatMultiObjectArray::validateFieldType(
    const std::string& fieldName,
    const ::value::Value& value
) const {
    auto field = classDefinition_->getField(fieldName);
    if (!field) {
        return false;
    }

    ::value::ValueType expectedType = field->getType();
    ::value::ValueType actualType = ::value::getValueType(value);

    return expectedType == actualType;
}

} // namespace arrays
} // namespace value
} // namespace mType
