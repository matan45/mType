#pragma once
#include "JsonValue.hpp"
#include "../value/ValueType.hpp"
#include "../environment/Environment.hpp"
#include <memory>

namespace json
{
    class JsonDeserializer
    {
    public:
        explicit JsonDeserializer(std::shared_ptr<environment::Environment> env);

        value::Value deserialize(const std::string& jsonString);
        value::Value deserializeAs(const std::string& jsonString,
                                    const std::string& className);

    private:
        std::shared_ptr<environment::Environment> environment;
        int currentDepth = 0;
        static constexpr int MAX_DEPTH = 64;

        value::Value fromJsonValue(const std::shared_ptr<JsonValue>& json);

        value::Value deserializeObject(const std::shared_ptr<JsonValue>& json);
        value::Value deserializeObjectAs(const std::shared_ptr<JsonValue>& json,
                                          const std::string& className);

        value::Value deserializeArray(const std::shared_ptr<JsonValue>& json,
                                       const std::string& elementType = "");

        void populateInstanceFields(
            std::shared_ptr<runtimeTypes::klass::ObjectInstance>& instance,
            const std::shared_ptr<runtimeTypes::klass::ClassDefinition>& classDef,
            const std::shared_ptr<JsonValue>& json);

        void populateStaticFields(
            const std::shared_ptr<runtimeTypes::klass::ClassDefinition>& classDef,
            const std::shared_ptr<JsonValue>& staticJson);

        void applyGenericBindings(
            std::shared_ptr<runtimeTypes::klass::ObjectInstance>& instance,
            const std::shared_ptr<JsonValue>& json);

        std::string resolveFieldType(
            const std::shared_ptr<runtimeTypes::klass::FieldDefinition>& field,
            const std::shared_ptr<runtimeTypes::klass::ObjectInstance>& instance);

        value::Value convertToFieldType(
            const std::shared_ptr<JsonValue>& json,
            const std::string& targetType);

        std::string resolveFieldTypeFromDef(
            const std::shared_ptr<runtimeTypes::klass::FieldDefinition>& field);
        static std::string valueTypeToString(value::ValueType type);

        struct DepthGuard
        {
            JsonDeserializer& deserializer;
            explicit DepthGuard(JsonDeserializer& d);
            ~DepthGuard();
        };
    };
}
