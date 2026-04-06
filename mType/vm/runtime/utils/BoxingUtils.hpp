#pragma once

#include "../../../value/ValueType.hpp"
#include "../../../value/ValueObject.hpp"
#include "../../../environment/Environment.hpp"
#include "../../../errors/RuntimeException.hpp"
#include "../../../runtimeTypes/klass/ClassDefinition.hpp"
#include <memory>

namespace vm::runtime::utils
{
    /**
     * Auto-box a raw primitive into its corresponding ValueObject at escape points.
     * Supports lazy re-boxing: INVOKE_INT_* opcodes push raw primitives,
     * and boxing only happens when the value needs to be used as an object.
     *
     * INVARIANT: Primitive wrapper classes (Int, Float, Bool) must be registered
     * as value classes with "value" at field index 0.
     */
    inline value::Value autoBoxPrimitive(const value::Value& val,
                                          const std::shared_ptr<environment::Environment>& env)
    {
        auto classRegistry = env->getClassRegistry();
        if (!classRegistry)
        {
            throw errors::RuntimeException(
                "Auto-boxing failed: class registry not available");
        }

        std::string className;
        if (std::holds_alternative<int64_t>(val))
            className = "Int";
        else if (std::holds_alternative<double>(val))
            className = "Float";
        else if (std::holds_alternative<bool>(val))
            className = "Bool";
        else
            return val;

        auto classDef = classRegistry->findClass(className);
        if (!classDef)
        {
            throw errors::RuntimeException(
                "Auto-boxing failed: " + className + " class not found — "
                "ensure lib/primitives/" + className + ".mt is loaded");
        }

        auto valueObj = std::make_shared<value::ValueObject>(classDef);
        valueObj->setFieldByIndex(0, val);
        return value::Value(valueObj);
    }
} // namespace vm::runtime::utils
