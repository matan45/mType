#include "ObjectArray.hpp"
#include "../../../runtimeTypes/klass/FieldDefinition.hpp"

namespace mType {
namespace value {
namespace arrays {

ObjectArray::ObjectArray(
    std::shared_ptr<runtimeTypes::klass::ClassDefinition> classDef,
    size_t initialSize
) : classDefinition_(classDef),
    size_(initialSize),
    capacity_(initialSize)
{
    if (!classDef) {
        throw std::invalid_argument("ClassDefinition cannot be null");
    }

    initializeFieldArrays();
}

void ObjectArray::initializeFieldArrays() {
    const auto& fields = classDefinition_->getInstanceFields();

    for (const auto& [fieldName, fieldDef] : fields) {
        auto fieldArray = createFieldArray(fieldDef, capacity_);
        fieldArrays_[fieldName] = fieldArray;
    }
}

std::shared_ptr<IArray> ObjectArray::createFieldArray(
    const std::shared_ptr<runtimeTypes::klass::FieldDefinition>& field,
    size_t capacity
) {
    ::value::ValueType fieldType = field->getType();

    // Determine field type and create appropriate array
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
            // Object types: Use generic IArray implementation
            // For now, we'll use a simple wrapper - could optimize later
            // This is for nested objects, which are less common
            return std::make_shared<IntArray>(capacity); // Placeholder - needs generic array impl
    }
}

// IArray implementation

size_t ObjectArray::size() const {
    return size_;
}

size_t ObjectArray::capacity() const {
    return capacity_;
}

bool ObjectArray::empty() const {
    return size_ == 0;
}

std::string ObjectArray::elementTypeName() const {
    return classDefinition_->getClassName();
}

::value::ValueType ObjectArray::elementType() const {
    return ::value::ValueType::OBJECT;
}

::value::Value ObjectArray::get(size_t index) const {
    if (index >= size_) {
        throw std::out_of_range("ObjectArray index out of bounds");
    }

    // Materialize ObjectInstance from SoA storage
    return materializeInstance(index);
}

void ObjectArray::set(size_t index, const ::value::Value& value) {
    if (index >= size_) {
        throw std::out_of_range("ObjectArray index out of bounds");
    }

    // Must be an ObjectInstance
    if (!std::holds_alternative<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(value)) {
        throw std::runtime_error("Value must be an ObjectInstance");
    }

    auto instance = std::get<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(value);

    // Validate it's the same class type
    if (instance->getClassDefinition()->getClassName() != classDefinition_->getClassName()) {
        throw std::runtime_error("ObjectInstance class mismatch");
    }

    // Decompose into SoA storage
    decomposeInstance(index, instance);
}

void ObjectArray::reserve(size_t newCapacity) {
    if (newCapacity <= capacity_) {
        return;
    }

    // Reserve capacity in all field arrays
    for (auto& [fieldName, fieldArray] : fieldArrays_) {
        fieldArray->reserve(newCapacity);
    }

    capacity_ = newCapacity;
}

void ObjectArray::resize(size_t newSize) {
    if (newSize == size_) {
        return;
    }

    // Resize all field arrays
    for (auto& [fieldName, fieldArray] : fieldArrays_) {
        fieldArray->resize(newSize);
    }

    size_ = newSize;

    if (size_ > capacity_) {
        capacity_ = size_;
    }
}

void ObjectArray::clear() {
    for (auto& [fieldName, fieldArray] : fieldArrays_) {
        fieldArray->clear();
    }

    size_ = 0;
}

bool ObjectArray::supportsSIMD() const {
    // ObjectArray supports SIMD if any field array supports it
    for (const auto& [fieldName, fieldArray] : fieldArrays_) {
        if (fieldArray->supportsSIMD()) {
            return true;
        }
    }
    return false;
}

size_t ObjectArray::simdWidth() const {
    // Return maximum SIMD width across all field arrays
    size_t maxWidth = 1;
    for (const auto& [fieldName, fieldArray] : fieldArrays_) {
        maxWidth = std::max(maxWidth, fieldArray->simdWidth());
    }
    return maxWidth;
}

std::unique_ptr<IArray> ObjectArray::clone() const {
    auto cloned = std::make_unique<ObjectArray>(classDefinition_, size_);

    // Clone all field arrays
    for (const auto& [fieldName, fieldArray] : fieldArrays_) {
        cloned->fieldArrays_[fieldName] = fieldArray->clone();
    }

    return cloned;
}

// ObjectArray-specific operations

::value::Value ObjectArray::getField(size_t index, const std::string& fieldName) const {
    if (index >= size_) {
        throw std::out_of_range("ObjectArray index out of bounds");
    }

    auto it = fieldArrays_.find(fieldName);
    if (it == fieldArrays_.end()) {
        throw std::runtime_error("Field not found: " + fieldName);
    }

    return it->second->get(index);
}

void ObjectArray::setField(size_t index, const std::string& fieldName, const ::value::Value& value) {
    if (index >= size_) {
        throw std::out_of_range("ObjectArray index out of bounds");
    }

    auto it = fieldArrays_.find(fieldName);
    if (it == fieldArrays_.end()) {
        throw std::runtime_error("Field not found: " + fieldName);
    }

    // Validate type
    if (!validateFieldType(fieldName, value)) {
        throw std::runtime_error("Field type mismatch for: " + fieldName);
    }

    it->second->set(index, value);
}

bool ObjectArray::hasField(const std::string& fieldName) const {
    return fieldArrays_.find(fieldName) != fieldArrays_.end();
}

std::vector<std::string> ObjectArray::getFieldNames() const {
    std::vector<std::string> names;
    names.reserve(fieldArrays_.size());

    for (const auto& [fieldName, fieldArray] : fieldArrays_) {
        names.push_back(fieldName);
    }

    return names;
}

std::shared_ptr<IArray> ObjectArray::getFieldArray(const std::string& fieldName) const {
    auto it = fieldArrays_.find(fieldName);
    if (it == fieldArrays_.end()) {
        return nullptr;
    }
    return it->second;
}

size_t ObjectArray::getMemoryUsage() const {
    size_t total = sizeof(ObjectArray);

    // Add field array memory
    for (const auto& [fieldName, fieldArray] : fieldArrays_) {
        // Field name string
        total += fieldName.capacity();

        // Field array data (approximate)
        total += fieldArray->capacity() * 8; // Rough estimate: 8 bytes per element
    }

    // Hash map overhead
    total += fieldArrays_.size() * 64; // Rough estimate for hash map nodes

    return total;
}

size_t ObjectArray::getMemorySavings() const {
    if (size_ == 0) {
        return 0;
    }

    // Calculate AoS memory: Each object has unordered_map overhead
    size_t aosMemory = size_ * (
        sizeof(runtimeTypes::klass::ObjectInstance) +
        64 + // shared_ptr overhead
        fieldArrays_.size() * (64 + 32) // hash map: node + key/value
    );

    size_t soaMemory = getMemoryUsage();

    return aosMemory > soaMemory ? aosMemory - soaMemory : 0;
}

ObjectArray::MemoryStats ObjectArray::getMemoryStats() const {
    MemoryStats stats;

    stats.objectCount = size_;
    stats.fieldCount = fieldArrays_.size();
    stats.soaMemoryUsage = getMemoryUsage();

    // Estimate AoS memory
    stats.aosMemoryUsage = size_ * (
        sizeof(runtimeTypes::klass::ObjectInstance) +
        64 + // shared_ptr overhead
        fieldArrays_.size() * (64 + 32) // hash map overhead per field
    );

    stats.memorySaved = stats.aosMemoryUsage > stats.soaMemoryUsage
        ? stats.aosMemoryUsage - stats.soaMemoryUsage
        : 0;

    stats.savingsPercentage = stats.aosMemoryUsage > 0
        ? (static_cast<double>(stats.memorySaved) / stats.aosMemoryUsage) * 100.0
        : 0.0;

    return stats;
}

// Private helper methods

std::shared_ptr<runtimeTypes::klass::ObjectInstance> ObjectArray::materializeInstance(size_t index) const {
    // Create new instance with class definition
    auto instance = std::make_shared<runtimeTypes::klass::ObjectInstance>(classDefinition_);

    // Populate fields from SoA storage
    for (const auto& [fieldName, fieldArray] : fieldArrays_) {
        ::value::Value fieldValue = fieldArray->get(index);
        instance->setField(fieldName, fieldValue);
    }

    return instance;
}

void ObjectArray::decomposeInstance(
    size_t index,
    const std::shared_ptr<runtimeTypes::klass::ObjectInstance>& instance
) {
    // Extract each field value and store in corresponding array
    for (const auto& [fieldName, fieldArray] : fieldArrays_) {
        ::value::Value fieldValue = instance->getFieldValue(fieldName);
        fieldArray->set(index, fieldValue);
    }
}

bool ObjectArray::validateFieldType(const std::string& fieldName, const ::value::Value& value) const {
    auto field = classDefinition_->getField(fieldName);
    if (!field) {
        return false;
    }

    ::value::ValueType expectedType = field->getType();
    ::value::ValueType actualType = ::value::getValueType(value);

    // Type checking
    return expectedType == actualType;
}

::value::ValueType ObjectArray::getFieldType(const std::string& fieldName) const {
    auto field = classDefinition_->getField(fieldName);
    if (!field) {
        return ::value::ValueType::VOID;
    }

    return field->getType();
}

} // namespace arrays
} // namespace value
} // namespace mType
