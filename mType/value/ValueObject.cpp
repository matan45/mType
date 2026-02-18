#include "ValueObject.hpp"
#include "../runtimeTypes/klass/ClassDefinition.hpp"

namespace value
{
    ValueObject::ValueObject(std::shared_ptr<runtimeTypes::klass::ClassDefinition> classDef)
        : classDefinition(std::move(classDef))
    {
        // Pre-allocate fields vector based on class field count
        if (classDefinition)
        {
            classDefinition->buildFieldIndexMap();
            size_t fieldCount = classDefinition->getIndexedFieldCount();
            fields.resize(fieldCount, nullptr_t{});
        }
    }

    ValueObject::ValueObject(const ValueObject& other)
        : classDefinition(other.classDefinition),
          fields(other.fields),
          genericTypeBindings(other.genericTypeBindings)
    {
    }

    ValueObject& ValueObject::operator=(const ValueObject& other)
    {
        if (this != &other)
        {
            classDefinition = other.classDefinition;
            fields = other.fields;
            genericTypeBindings = other.genericTypeBindings;
        }
        return *this;
    }

    const Value& ValueObject::getFieldByIndex(size_t index) const
    {
        return fields[index];
    }

    void ValueObject::setFieldByIndex(size_t index, const Value& value)
    {
        if (index < fields.size())
        {
            fields[index] = value;
        }
    }

    Value ValueObject::getFieldValue(const std::string& fieldName) const
    {
        if (!classDefinition)
        {
            return nullptr_t{};
        }

        size_t index = classDefinition->getFieldIndex(fieldName);
        if (index < fields.size())
        {
            return fields[index];
        }
        return nullptr_t{};
    }

    void ValueObject::setField(const std::string& fieldName, const Value& value)
    {
        if (!classDefinition)
        {
            return;
        }

        size_t index = classDefinition->getFieldIndex(fieldName);
        if (index < fields.size())
        {
            fields[index] = value;
        }
    }

    bool ValueObject::hasField(const std::string& fieldName) const
    {
        if (!classDefinition)
        {
            return false;
        }

        // getFieldIndex returns a sentinel (size_t max) or throws if not found
        // Check via the field index map directly
        const auto& indexMap = classDefinition->getFieldIndexMap();
        return indexMap.find(fieldName) != indexMap.end();
    }

    std::shared_ptr<runtimeTypes::klass::ClassDefinition> ValueObject::getClassDefinition() const
    {
        return classDefinition;
    }

    const std::string& ValueObject::getClassName() const
    {
        static const std::string empty;
        return classDefinition ? classDefinition->getClassName() : empty;
    }

    void ValueObject::setGenericTypeBinding(const std::string& param, const std::string& type)
    {
        genericTypeBindings[param] = type;
    }

    const std::unordered_map<std::string, std::string>& ValueObject::getGenericTypeBindings() const
    {
        return genericTypeBindings;
    }

    std::shared_ptr<ValueObject> ValueObject::deepCopy() const
    {
        auto copy = std::make_shared<ValueObject>(*this);
        return copy;
    }

    bool ValueObject::equals(const ValueObject& other) const
    {
        // Must be same class type
        if (!classDefinition || !other.classDefinition)
        {
            return classDefinition == other.classDefinition;
        }
        if (classDefinition->getClassName() != other.classDefinition->getClassName())
        {
            return false;
        }

        // Structural equality: compare all fields
        if (fields.size() != other.fields.size())
        {
            return false;
        }
        for (size_t i = 0; i < fields.size(); ++i)
        {
            if (fields[i] != other.fields[i])
            {
                return false;
            }
        }
        return true;
    }

    size_t ValueObject::getFieldCount() const
    {
        return fields.size();
    }

    const std::vector<Value>& ValueObject::getFields() const
    {
        return fields;
    }
}
