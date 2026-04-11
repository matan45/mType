#pragma once
#include <memory>
#include <string>
#include <variant>
#include "../../value/ValueType.hpp"
#include "../../runtimeTypes/klass/ObjectInstance.hpp"
#include "../../runtimeTypes/klass/ClassDefinition.hpp"
#include "../../errors/ObjectException.hpp"

namespace services::api
{
    /**
     * Typed FFI wrapper around a value::Value that holds a runtime mType Class
     * instance (i.e. an ObjectInstance whose ClassDefinition is the reflection
     * "Class" class). Constructing one is the single chokepoint that validates
     * the inner variant — native FFI authors should never have to unwrap
     * std::shared_ptr<ObjectInstance> themselves.
     *
     * Class values round-trip through value::Value unchanged: an api::Class
     * constructed from a Value returned by ScriptAPI can be passed straight
     * back into another ScriptAPI call via asValue().
     *
     * This wrapper is intentionally thin — convenience introspection (getName,
     * getTypeArguments, etc.) is NOT duplicated here. Call them through
     * ScriptAPI::callMethod(cls.asValue(), "getName", {}) so there is exactly
     * one implementation for every operation.
     */
    class Class
    {
    private:
        value::Value value_;

    public:
        /**
         * Construct from a value::Value. Throws ObjectException if the value
         * does not hold a Class ObjectInstance.
         */
        explicit Class(value::Value v)
            : value_(std::move(v))
        {
            if (!isClassValue(value_))
            {
                throw errors::ObjectException(
                    "Value is not a Class instance", "", "api::Class::Class");
            }
        }

        /**
         * Access the underlying value::Value for passing back through
         * ScriptAPI. Identity is preserved — two api::Class values constructed
         * from the same interned Class ObjectInstance share the same shared_ptr.
         */
        const value::Value& asValue() const { return value_; }

        /**
         * Runtime check: is this Value a Class ObjectInstance?
         *
         * Two signals are required — the class name must be "Class" AND
         * the instance must carry a `_nativeHandle: int` field. This
         * guards against user code defining `class Class {}` as an
         * impostor — such a class would lack the reflection handle and
         * is rejected at the wrapper boundary instead of silently
         * sailing through with undefined behavior.
         *
         * The field-existence check is what gives the guarantee teeth.
         * A plain name check would accept any user-defined shadow.
         */
        static bool isClassValue(const value::Value& v)
        {
            if (!std::holds_alternative<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(v))
            {
                return false;
            }
            const auto& instance = std::get<std::shared_ptr<runtimeTypes::klass::ObjectInstance>>(v);
            if (!instance)
            {
                return false;
            }
            auto classDef = instance->getClassDefinition();
            if (!classDef || classDef->getName() != "Class")
            {
                return false;
            }
            // Second signal: the reflection Class carries a
            // `_nativeHandle: int` field that no user-defined "Class"
            // shadow would have by accident.
            auto handleField = instance->getField("_nativeHandle");
            return handleField != nullptr;
        }
    };
}
