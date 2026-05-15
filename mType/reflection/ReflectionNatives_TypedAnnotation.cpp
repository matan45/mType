#include "ReflectionNatives.hpp"
#include <cstddef>
#include <cstdint>
#include "ReflectionHandle.hpp"
#include "AnnotationInstanceFactory.hpp"
#include "../value/NativeArray.hpp"
#include "../value/InternedString.hpp"
#include "../errors/RuntimeException.hpp"
#include "../runtimeTypes/klass/FieldDefinition.hpp"
#include "../runtimeTypes/klass/AnnotationDefinition.hpp"
#include "../ast/nodes/annotations/TypedAnnotationValue.hpp"
#include "../environment/Environment.hpp"
#include <sstream>

namespace reflection
{
    using namespace value;
    using ast::nodes::annotations::AnnotationValueType;
    using ast::nodes::annotations::TypedAnnotationValue;

    namespace
    {
        const TypedAnnotationValue* fetchTypedParam(void* userData, environment::NativeContext& ctx, std::span<const value::Value> args, const char* fn)
        {
            if (args.size() != 2)
                throw errors::RuntimeException(std::string(fn) + " requires 2 arguments");
            if (!isInt(args[0]))
                throw errors::RuntimeException(std::string(fn) + " requires annotationHandle:int");
            int64_t handle = asInt(args[0]);
            std::string key;
            // MYT-317: SSO-aware.
            if (isAnyString(args[1])) key = std::string(asStringView(args[1]));
            else throw errors::RuntimeException(std::string(fn) + " requires paramKey:string");

            auto info = ReflectionHandleRegistry::instance().getAnnotation(handle);
            if (!info.annotation)
                throw errors::RuntimeException("Invalid annotation handle");
            return info.annotation->getTypedParameter(key);
        }

        void requireType(const TypedAnnotationValue* v, AnnotationValueType expected, const char* fn, const char* expectedName)
        {
            if (!v) throw errors::RuntimeException(std::string(fn) + ": parameter not present");
            if (v->getType() != expected)
                throw errors::RuntimeException(std::string(fn) + ": parameter is not " + expectedName);
        }
    }

    Value ReflectionNatives::__reflect_getAnnotationInt(void* userData, environment::NativeContext& ctx, std::span<const value::Value> args)
    {
        const auto* v = fetchTypedParam(userData, ctx, args, "__reflect_getAnnotationInt");
        requireType(v, AnnotationValueType::INT, "__reflect_getAnnotationInt", "int");
        return v->asInt();
    }

    Value ReflectionNatives::__reflect_getAnnotationFloat(void* userData, environment::NativeContext& ctx, std::span<const value::Value> args)
    {
        const auto* v = fetchTypedParam(userData, ctx, args, "__reflect_getAnnotationFloat");
        if (!v) throw errors::RuntimeException("__reflect_getAnnotationFloat: parameter not present");
        if (v->getType() == AnnotationValueType::FLOAT) return v->asFloat();
        // Forgiving widening — int annotation values are accepted as float.
        if (v->getType() == AnnotationValueType::INT) return static_cast<double>(v->asInt());
        throw errors::RuntimeException("__reflect_getAnnotationFloat: parameter is not float");
    }

    Value ReflectionNatives::__reflect_getAnnotationBool(void* userData, environment::NativeContext& ctx, std::span<const value::Value> args)
    {
        const auto* v = fetchTypedParam(userData, ctx, args, "__reflect_getAnnotationBool");
        requireType(v, AnnotationValueType::BOOL, "__reflect_getAnnotationBool", "bool");
        return v->asBool();
    }

    Value ReflectionNatives::__reflect_getAnnotationString(void* userData, environment::NativeContext& ctx, std::span<const value::Value> args)
    {
        const auto* v = fetchTypedParam(userData, ctx, args, "__reflect_getAnnotationString");
        requireType(v, AnnotationValueType::STRING, "__reflect_getAnnotationString", "string");
        return v->asString();
    }

    Value ReflectionNatives::__reflect_getAnnotationClass(void* userData, environment::NativeContext& ctx, std::span<const value::Value> args)
    {
        const auto* v = fetchTypedParam(userData, ctx, args, "__reflect_getAnnotationClass");
        requireType(v, AnnotationValueType::CLASS_REF, "__reflect_getAnnotationClass", "Class");
        return v->asClassRef();
    }

    Value ReflectionNatives::__reflect_getAnnotationClassArray(void* userData, environment::NativeContext& ctx, std::span<const value::Value> args)
    {
        const auto* v = fetchTypedParam(userData, ctx, args, "__reflect_getAnnotationClassArray");
        if (!v) throw errors::RuntimeException("__reflect_getAnnotationClassArray: parameter not present");
        std::vector<std::string> names;
        if (v->getType() == AnnotationValueType::CLASS_ARRAY) names = v->asClassArray();
        else if (v->getType() == AnnotationValueType::CLASS_REF) names.push_back(v->asClassRef());
        else throw errors::RuntimeException("__reflect_getAnnotationClassArray: parameter is not Class[]");

        if (!ctx.env) throw errors::RuntimeException("ReflectionNatives: environment not initialized");
        auto reg = ctx.env->getClassRegistry();
        auto result = std::make_shared<NativeArray>(names.size(), ValueType::INT);
        for (size_t i = 0; i < names.size(); ++i)
        {
            auto def = reg->findClass(names[i]);
            if (!def) throw errors::RuntimeException("Annotation Class[] contains unknown class: " + names[i]);
            result->set(static_cast<int>(i), ReflectionHandleRegistry::instance().getOrCreateClassHandle(def));
        }
        return result;
    }

    Value ReflectionNatives::__reflect_isAnnotationParamNull(void* userData, environment::NativeContext& ctx, std::span<const value::Value> args)
    {
        const auto* v = fetchTypedParam(userData, ctx, args, "__reflect_isAnnotationParamNull");
        // Null values in annotations are represented as missing or NULL_TYPE TypedAnnotationValue
        return v == nullptr || v->getType() == AnnotationValueType::NULL_VALUE;
    }

    Value ReflectionNatives::__reflect_getAnnotationObject(void* userData, environment::NativeContext& ctx, std::span<const value::Value> args)
    {
        validateArgCount(args, 2, "__reflect_getAnnotationObject");
        int64_t targetHandle = extractInt(args[0], "__reflect_getAnnotationObject", "targetHandle");
        int64_t classHandle = extractInt(args[1], "__reflect_getAnnotationObject", "annotationClassHandle");

        auto& registry = ReflectionHandleRegistry::instance();
        auto target = registry.getAnnotation(targetHandle);
        auto classDef = registry.getClass(classHandle);

        if (!target.annotation || !classDef)
        {
            throw errors::RuntimeException("Invalid handles for __reflect_getAnnotationObject");
        }

        if (!ctx.env) throw errors::RuntimeException("ReflectionNatives: environment not initialized");

        // Find the AnnotationDefinition matching the target annotation's name
        auto annDef = ctx.env->getAnnotationRegistry()->findAnnotation(target.annotationName);
        if (!annDef)
        {
            throw errors::RuntimeException("Annotation definition not found: " + target.annotationName);
        }

        auto inst = AnnotationInstanceFactory::buildInstance(annDef, target.annotation);
        if (!inst)
        {
            return std::monostate{};
        }
        return inst;
    }
}

