#pragma once
#include "JsonValue.hpp"
#include "../value/ValueType.hpp"
#include "../environment/Environment.hpp"
#include <unordered_set>
#include <memory>

namespace json
{
    struct SerializationOptions
    {
        bool includeStaticFields = false;
        bool prettyPrint = true;
        int maxDepth = 64;
    };

    class JsonSerializer
    {
    public:
        explicit JsonSerializer(std::shared_ptr<environment::Environment> env,
                               const SerializationOptions& options = {});

        std::string serialize(const value::Value& val);
        std::shared_ptr<JsonValue> toJsonValue(const value::Value& val);

    private:
        std::shared_ptr<environment::Environment> environment;
        SerializationOptions options;
        std::unordered_set<const void*> visitedObjects;
        int currentDepth = 0;

        std::shared_ptr<JsonValue> serializeValue(const value::Value& val);
        std::shared_ptr<JsonValue> serializeObject(
            const std::shared_ptr<runtimeTypes::klass::ObjectInstance>& obj);
        std::shared_ptr<JsonValue> serializeValueObject(
            const std::shared_ptr<value::ValueObject>& obj);
        std::shared_ptr<JsonValue> serializeArray(
            const std::shared_ptr<value::NativeArray>& arr);

        void addStaticFields(
            const std::shared_ptr<runtimeTypes::klass::ClassDefinition>& classDef,
            std::shared_ptr<JsonValue>& jsonObj);
        void addGenericBindings(
            const std::unordered_map<std::string, std::string>& bindings,
            std::shared_ptr<JsonValue>& jsonObj);

        struct DepthGuard
        {
            JsonSerializer& serializer;
            explicit DepthGuard(JsonSerializer& s);
            ~DepthGuard();
        };

        struct CycleGuard
        {
            std::unordered_set<const void*>& visited;
            const void* ptr;
            CycleGuard(std::unordered_set<const void*>& visited, const void* ptr);
            ~CycleGuard();
        };
    };
}
