#include "ScriptAPI.hpp"
#include <cstdint>
#include "../environment/registry/ClassDefinition.hpp"
#include "../value/ObjectInstance.hpp"
#include "../value/ValueShim.hpp"
#include "../value/NativeArray.hpp"
#include "../errors/ObjectException.hpp"
#include "../errors/RuntimeException.hpp"
#include "../vm/runtime/executors/TypeExecutor.hpp"
#include "../reflection/ReflectionHandle.hpp"
#include "../types/UnifiedType.hpp"

namespace services
{
    namespace
    {
        std::shared_ptr<runtimeTypes::klass::ObjectInstance>
        asObjectInstance(const value::Value& v, const char* apiName)
        {
            if (value::isStackObject(v))
            {
                auto* raw = value::asObjectInstanceRaw(v);
                if (!raw)
                {
                    throw errors::ObjectException(
                        "Value is a null object", "", apiName);
                }
                return std::shared_ptr<runtimeTypes::klass::ObjectInstance>(
                    raw, [](runtimeTypes::klass::ObjectInstance*) {});
            }
            if (!value::isObject(v))
            {
                throw errors::ObjectException(
                    "Value is not an object", "", apiName);
            }
            auto instance = value::asObject(v);
            if (!instance)
            {
                throw errors::ObjectException(
                    "Value is a null object", "", apiName);
            }
            return instance;
        }
    }

    api::Class ScriptAPI::getClass(const value::Value& object)
    {
        auto instance = asObjectInstance(object, "ScriptAPI::getClass");

        // Reconstruct the canonical parameterized name using the same
        // helper the bytecode INSTANCEOF op uses — this guarantees the
        // string we pass to Class.forName lines up with what the language
        // side would produce (e.g. "Box<Int>" with ", " spacing).
        std::string canonicalName =
            vm::runtime::TypeExecutor::reconstructObjectFullType(instance.get());
        if (canonicalName.empty())
        {
            canonicalName = instance->getClassDefinition()->getName();
        }

        // Route through the same reflection path as language-side
        // Class.forName — same ReflectionHandleRegistry, same interning,
        // same handle identity.
        value::Value raw = callStaticMethod(
            "Class", "forName", { value::Value(canonicalName) });
        return api::Class(std::move(raw));
    }

    api::Class ScriptAPI::classOf(const std::string& typeName)
    {
        value::Value raw = callStaticMethod(
            "Class", "forName", { value::Value(typeName) });
        return api::Class(std::move(raw));
    }

    std::vector<api::Class> ScriptAPI::getGenericArguments(const value::Value& object)
    {
        // Route through the language surface: Class.getTypeArguments()
        // returns Class[] via the same __reflect_getTypeArguments native
        // that powers the language path, so identity is preserved.
        api::Class cls = getClass(object);
        value::Value argsArray = callMethod(cls.asValue(), "getTypeArguments", {});

        // Class.getTypeArguments() always returns a Class[] — at the VM
        // level a shared_ptr<NativeArray> with object elements. Anything
        // else is an internal invariant violation and must fail loudly
        // rather than silently return an empty vector.
        if (!value::isNativeArray(argsArray))
        {
            throw errors::ObjectException(
                "Class.getTypeArguments() returned an unexpected value shape "
                "(expected NativeArray)",
                "Class", "ScriptAPI::getGenericArguments");
        }
        auto array = value::asNativeArray(argsArray);
        if (!array)
        {
            throw errors::ObjectException(
                "Class.getTypeArguments() returned a null array",
                "Class", "ScriptAPI::getGenericArguments");
        }

        std::vector<api::Class> result;
        result.reserve(array->size());
        for (size_t i = 0; i < array->size(); ++i)
        {
            result.emplace_back(array->get(i));
        }
        return result;
    }

    bool ScriptAPI::isInstanceOf(const value::Value& object,
                                 const std::string& fullyParameterizedType)
    {
        // Shared code path with bytecode INSTANCEOF — same routine in
        // TypeExecutor handles primitives, parameterized objects, and the
        // interface hierarchy walk.
        return vm::runtime::TypeExecutor::checkInstanceOfByName(
            object, fullyParameterizedType, environment);
    }

    bool ScriptAPI::isInstanceOf(const value::Value& object, const api::Class& cls)
    {
        // Hot-path implementation — read the Class's `_nativeHandle`
        // field directly and resolve the canonical name through
        // ReflectionHandleRegistry. No VM round-trip, no bytecode
        // invocation. For closed handles (parameterized types) we use
        // the reified UnifiedType's canonical string; for open handles
        // we use the ClassDefinition's raw name.
        auto instance = asObjectInstance(cls.asValue(), "ScriptAPI::isInstanceOf");
        value::Value handleVal = instance->getFieldValue("_nativeHandle");
        if (!value::isInt(handleVal))
        {
            throw errors::ObjectException(
                "Class._nativeHandle is missing or not an int",
                "Class", "ScriptAPI::isInstanceOf");
        }
        int64_t handle = value::asInt(handleVal);

        auto& registry = reflection::ReflectionHandleRegistry::instance();
        std::string name;
        auto reified = registry.getReifiedType(handle);
        if (reified)
        {
            name = reified->toCanonicalString();
        }
        else
        {
            auto classDef = registry.getClass(handle);
            if (!classDef)
            {
                throw errors::RuntimeException(
                    "ScriptAPI::isInstanceOf: invalid class handle");
            }
            name = classDef->getName();
        }
        return isInstanceOf(object, name);
    }
}
