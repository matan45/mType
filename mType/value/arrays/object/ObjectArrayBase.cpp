#include "ObjectArrayBase.hpp"
#include "../../../runtimeTypes/klass/FieldDefinition.hpp"

namespace mType {
namespace value {
namespace arrays {

ObjectArrayBase::ObjectArrayBase(
    std::shared_ptr<runtimeTypes::klass::ClassDefinition> classDef
) : classDefinition_(classDef)
{
    if (!classDef)
    {
        throw std::invalid_argument("ClassDefinition cannot be null");
    }
}

void ObjectArrayBase::initializeFieldArrays(size_t capacity)
{
    const auto& fields = classDefinition_->getInstanceFields();

    for (const auto& [fieldName, fieldDef] : fields)
    {
        auto fieldArray = createFieldArray(fieldDef, capacity);
        fieldArrays_[fieldName] = fieldArray;
    }
}

std::shared_ptr<IArray> ObjectArrayBase::createFieldArray(
    const std::shared_ptr<runtimeTypes::klass::FieldDefinition>& field,
    size_t capacity
)
{
    ::value::ValueType fieldType = field->getType();

    // Determine field type and create appropriate array
    switch (fieldType)
    {
    case ::value::ValueType::INT:
        return std::make_shared<IntArray>(capacity);
    case ::value::ValueType::FLOAT:
        return std::make_shared<FloatArray>(capacity);
    case ::value::ValueType::BOOL:
        return std::make_shared<BoolArray>(capacity);
    case ::value::ValueType::STRING:
        return std::make_shared<StringArray>(capacity);
    case ::value::ValueType::OBJECT:
    case ::value::ValueType::ARRAY:
    case ::value::ValueType::VOID:
    default:
        // Nested objects, arrays, and other complex types are not yet supported
        throw std::runtime_error(
            "Nested objects/arrays in ObjectArray fields are not yet supported. "
            "Field type: " + std::to_string(static_cast<int>(fieldType))
        );
    }
}

std::shared_ptr<runtimeTypes::klass::ObjectInstance> ObjectArrayBase::materializeInstance(
    size_t linearIndex
) const
{
    // Create new instance with class definition
    auto instance = std::make_shared<runtimeTypes::klass::ObjectInstance>(classDefinition_);

    // Populate fields from SoA storage
    for (const auto& [fieldName, fieldArray] : fieldArrays_)
    {
        ::value::Value fieldValue = fieldArray->get(linearIndex);
        instance->setField(fieldName, fieldValue);
    }

    return instance;
}

void ObjectArrayBase::decomposeInstance(
    size_t linearIndex,
    const std::shared_ptr<runtimeTypes::klass::ObjectInstance>& instance
)
{
    // Extract each field value and store in corresponding array
    for (const auto& [fieldName, fieldArray] : fieldArrays_)
    {
        ::value::Value fieldValue = instance->getFieldValue(fieldName);
        fieldArray->set(linearIndex, fieldValue);
    }
}

bool ObjectArrayBase::validateFieldType(
    const std::string& fieldName,
    const ::value::Value& value
) const
{
    auto field = classDefinition_->getField(fieldName);
    if (!field)
    {
        return false;
    }

    ::value::ValueType expectedType = field->getType();
    ::value::ValueType actualType = ::value::ValueTypeUtils::getValueType(value);

    // Type checking
    return expectedType == actualType;
}

bool ObjectArrayBase::hasField(const std::string& fieldName) const
{
    return fieldArrays_.find(fieldName) != fieldArrays_.end();
}

std::vector<std::string> ObjectArrayBase::getFieldNames() const
{
    std::vector<std::string> names;
    names.reserve(fieldArrays_.size());

    for (const auto& [fieldName, fieldArray] : fieldArrays_)
    {
        names.push_back(fieldName);
    }

    return names;
}

std::shared_ptr<IArray> ObjectArrayBase::getFieldArray(const std::string& fieldName) const
{
    auto it = fieldArrays_.find(fieldName);
    if (it == fieldArrays_.end())
    {
        return nullptr;
    }
    return it->second;
}

bool ObjectArrayBase::supportsSIMD() const
{
    // ObjectArray supports SIMD if any field array supports it
    for (const auto& [fieldName, fieldArray] : fieldArrays_)
    {
        if (fieldArray->supportsSIMD())
        {
            return true;
        }
    }
    return false;
}

} // namespace arrays
} // namespace value
} // namespace mType
