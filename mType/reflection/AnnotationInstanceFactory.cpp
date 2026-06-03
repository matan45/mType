#include "AnnotationInstanceFactory.hpp"
#include "../environment/registry/FieldDefinition.hpp"
#include "../environment/registry/ClassDefinition.hpp"
#include "../value/ObjectInstancePool.hpp"
#include "../value/NativeArray.hpp"
#include <vector>

namespace reflection
{
    using namespace runtimeTypes::klass;
    using ast::nodes::annotations::AnnotationValueType;
    using ast::nodes::annotations::TypedAnnotationValue;

    namespace
    {
        value::ValueType toValueType(AnnotationValueType t)
        {
            switch (t)
            {
            case AnnotationValueType::INT:         return value::ValueType::INT;
            case AnnotationValueType::FLOAT:       return value::ValueType::FLOAT;
            case AnnotationValueType::BOOL:        return value::ValueType::BOOL;
            case AnnotationValueType::STRING:      return value::ValueType::STRING;
            case AnnotationValueType::CLASS_REF:   return value::ValueType::STRING; // Class ref stored as name
            case AnnotationValueType::CLASS_ARRAY: return value::ValueType::ARRAY;
            case AnnotationValueType::INT_ARRAY:   return value::ValueType::ARRAY;
            case AnnotationValueType::FLOAT_ARRAY: return value::ValueType::ARRAY;
            case AnnotationValueType::BOOL_ARRAY:  return value::ValueType::ARRAY;
            case AnnotationValueType::STRING_ARRAY:return value::ValueType::ARRAY;
            case AnnotationValueType::NULL_VALUE:  return value::ValueType::NULL_TYPE;
            }
            return value::ValueType::OBJECT;
        }

        template<typename T>
        value::Value makeNativeArray(const std::vector<T>& values, value::ValueType elementType)
        {
            auto result = std::make_shared<value::NativeArray>(values.size(), elementType);
            for (size_t i = 0; i < values.size(); ++i)
            {
                T item = values[i];
                result->set(i, item);
            }
            return result;
        }

        value::Value toRuntimeValue(const TypedAnnotationValue& v)
        {
            switch (v.getType())
            {
            case AnnotationValueType::NULL_VALUE:  return std::monostate{};
            case AnnotationValueType::INT:         return v.asInt();
            case AnnotationValueType::FLOAT:       return v.asFloat();
            case AnnotationValueType::BOOL:        return v.asBool();
            case AnnotationValueType::STRING:      return v.asString();
            case AnnotationValueType::CLASS_REF:   return v.asClassRef();
            case AnnotationValueType::CLASS_ARRAY:
                return makeNativeArray(v.asClassArray(), value::ValueType::STRING);
            case AnnotationValueType::INT_ARRAY:
                return makeNativeArray(v.asIntArray(), value::ValueType::INT);
            case AnnotationValueType::FLOAT_ARRAY:
                return makeNativeArray(v.asFloatArray(), value::ValueType::FLOAT);
            case AnnotationValueType::BOOL_ARRAY:
                return makeNativeArray(v.asBoolArray(), value::ValueType::BOOL);
            case AnnotationValueType::STRING_ARRAY:
                return makeNativeArray(v.asStringArray(), value::ValueType::STRING);
            }
            return std::monostate{};
        }
    }

    std::shared_ptr<ClassDefinition>
        AnnotationInstanceFactory::getOrCreateSyntheticClass(std::shared_ptr<AnnotationDefinition> def)
    {
        if (!def) return nullptr;
        if (auto cached = def->getSyntheticClass()) return cached;

        auto cls = std::make_shared<ClassDefinition>(def->getName());
        for (const auto& schema : def->getParams())
        {
            auto field = std::make_shared<FieldDefinition>(
                schema.name,
                toValueType(schema.declaredType),
                value::Value{},          // initial value
                /*static=*/false,
                /*final=*/true,
                ast::AccessModifier::PUBLIC);
            cls->addInstanceField(schema.name, field);
        }
        def->setSyntheticClass(cls);
        return cls;
    }

    std::shared_ptr<ObjectInstance>
        AnnotationInstanceFactory::buildInstance(
            std::shared_ptr<AnnotationDefinition> def,
            std::shared_ptr<ast::nodes::annotations::AnnotationNode> annotation)
    {
        auto cls = getOrCreateSyntheticClass(def);
        if (!cls || !annotation) return nullptr;

        auto instance = value::ObjectInstancePool::getInstance().acquire(cls);
        for (const auto& schema : def->getParams())
        {
            const TypedAnnotationValue* value = annotation->getTypedParameter(schema.name);
            if (!value && schema.defaultValue.has_value())
            {
                value = &(*schema.defaultValue);
            }
            if (!value) continue;
            instance->setField(schema.name, toRuntimeValue(*value));
        }
        return instance;
    }
}
