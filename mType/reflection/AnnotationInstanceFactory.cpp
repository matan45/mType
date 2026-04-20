#include "AnnotationInstanceFactory.hpp"
#include "../runtimeTypes/klass/FieldDefinition.hpp"
#include "../runtimeTypes/klass/ClassDefinition.hpp"
#include "../value/ObjectInstancePool.hpp"

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
            case AnnotationValueType::NULL_VALUE:  return value::ValueType::NULL_TYPE;
            }
            return value::ValueType::OBJECT;
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
                // Returned as a comma-joined string for now — array materialization
                // requires NativeArray construction with element-typed handles.
                return v.toDisplayString();
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
