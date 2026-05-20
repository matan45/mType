#pragma once

#include "../../../value/ValueType.hpp"
#include "../../../value/ValueObject.hpp"
#include "../../../environment/Environment.hpp"
#include "../../../errors/RuntimeException.hpp"
#include "../../../environment/registry/ClassDefinition.hpp"
#include "../../bytecode/BytecodeProgram.hpp"
#include <cstdint>
#include <memory>
#include <span>
#include <string>

namespace vm::runtime::utils
{
    inline bool primitiveWrapperArgMatches(value::PrimitiveTypeTag tag,
                                           const value::Value& val)
    {
        switch (tag)
        {
        case value::PrimitiveTypeTag::INT:
            return value::isInt(val);
        case value::PrimitiveTypeTag::FLOAT:
            return value::isFloat(val);
        case value::PrimitiveTypeTag::BOOL:
            return value::isBool(val);
        case value::PrimitiveTypeTag::STRING:
            // MYT-317: STRING_INLINE counts as a string-typed primitive.
            return value::isAnyString(val);
        case value::PrimitiveTypeTag::NONE:
            return false;
        }
        return false;
    }

    inline bool tryCreatePrimitiveValueObject(
        const std::string& className,
        std::span<const value::Value> args,
        environment::Environment* env,
        value::Value& out)
    {
        if (args.size() != 1 || !env)
            return false;

        const value::PrimitiveTypeTag tag = value::classNameToPrimitiveTag(className);
        if (tag == value::PrimitiveTypeTag::NONE ||
            !primitiveWrapperArgMatches(tag, args[0]))
        {
            return false;
        }

        auto classRegistry = env->getClassRegistry();
        auto classDef = classRegistry ? classRegistry->findClass(className) : nullptr;
        if (!classDef || !classDef->isValueClass())
            return false;

        classDef->buildFieldIndexMap();
        if (classDef->getFieldIndex("value") != 0)
            return false;

        auto valueObj = std::make_shared<value::ValueObject>(classDef);
        valueObj->setFieldByIndex(0, args[0]);
        out = value::Value(std::move(valueObj));
        return true;
    }

    // MYT-TBD (box/unbox perf): cached variant of tryCreatePrimitiveValueObject
    // for the JIT NEW_VALUE_OBJECT hot path. The first call at a given
    // classIndex resolves (className -> ClassDefinition, PrimitiveTypeTag,
    // valueFieldAtIndex0) and stashes the result on
    // BytecodeProgram::primitiveWrapperCache; subsequent calls skip the
    // string-based dispatch, ClassRegistry hashmap, and getFieldIndex
    // hashmap entirely. On any resolution failure the slot is still marked
    // resolved with tag=NONE so we don't retry.
    //
    // Mirrors the (programTag, funcMetadata) caching pattern used by
    // ConstructorDefinition::CallSiteCache for jit_new_object -> createObject.
    inline bool tryCreatePrimitiveValueObjectCached(
        const vm::bytecode::BytecodeProgram* program,
        uint32_t classIndex,
        std::span<const value::Value> args,
        environment::Environment* env,
        value::Value& out)
    {
        if (args.size() != 1 || !env || !program)
            return false;

        auto& cache = program->primitiveWrapperCache;
        if (classIndex >= cache.size())
        {
            cache.resize(program->getConstantPool().strings.size());
        }
        auto& entry = cache[classIndex];

        if (!entry.resolved)
        {
            const std::string& className =
                program->getConstantPool().getString(classIndex);
            entry.tag = value::classNameToPrimitiveTag(className);

            if (entry.tag != value::PrimitiveTypeTag::NONE)
            {
                auto classRegistry = env->getClassRegistry();
                auto classDef = classRegistry ? classRegistry->findClass(className) : nullptr;
                if (classDef && classDef->isValueClass())
                {
                    classDef->buildFieldIndexMap();
                    if (classDef->getFieldIndex("value") == 0)
                    {
                        entry.classDef = std::move(classDef);
                    }
                    else
                    {
                        entry.tag = value::PrimitiveTypeTag::NONE;
                    }
                }
                else
                {
                    entry.tag = value::PrimitiveTypeTag::NONE;
                }
            }
            entry.resolved = true;
        }

        if (entry.tag == value::PrimitiveTypeTag::NONE || !entry.classDef)
            return false;
        if (!primitiveWrapperArgMatches(entry.tag, args[0]))
            return false;

        auto valueObj = std::make_shared<value::ValueObject>(entry.classDef);
        valueObj->setFieldByIndex(0, args[0]);
        out = value::Value(std::move(valueObj));
        return true;
    }

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
        if (value::isInt(val))
            className = "Int";
        else if (value::isFloat(val))
            className = "Float";
        else if (value::isBool(val))
            className = "Bool";
        else if (value::isAnyString(val))  // MYT-317: includes STRING_INLINE.
            className = "String";
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
