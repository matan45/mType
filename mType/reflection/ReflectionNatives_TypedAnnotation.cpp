#include "ReflectionNatives.hpp"
#include "ReflectionHandle.hpp"
#include "AnnotationInstanceFactory.hpp"
#include "../value/NativeArray.hpp"
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
        const TypedAnnotationValue* fetchTypedParam(const std::vector<Value>& args, const char* fn)
        {
            if (args.size() != 2)
                throw errors::RuntimeException(std::string(fn) + " requires 2 arguments");
            if (!std::holds_alternative<int64_t>(args[0]))
                throw errors::RuntimeException(std::string(fn) + " requires annotationHandle:int");
            int64_t handle = std::get<int64_t>(args[0]);
            std::string key;
            if (std::holds_alternative<std::string>(args[1])) key = std::get<std::string>(args[1]);
            else if (std::holds_alternative<InternedString>(args[1])) key = std::get<InternedString>(args[1]).getString();
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

    Value ReflectionNatives::__reflect_getAnnotationInt(const std::vector<Value>& args)
    {
        const auto* v = fetchTypedParam(args, "__reflect_getAnnotationInt");
        requireType(v, AnnotationValueType::INT, "__reflect_getAnnotationInt", "int");
        return v->asInt();
    }

    Value ReflectionNatives::__reflect_getAnnotationFloat(const std::vector<Value>& args)
    {
        const auto* v = fetchTypedParam(args, "__reflect_getAnnotationFloat");
        if (!v) throw errors::RuntimeException("__reflect_getAnnotationFloat: parameter not present");
        if (v->getType() == AnnotationValueType::FLOAT) return v->asFloat();
        // Forgiving widening — int annotation values are accepted as float.
        if (v->getType() == AnnotationValueType::INT) return static_cast<double>(v->asInt());
        throw errors::RuntimeException("__reflect_getAnnotationFloat: parameter is not float");
    }

    Value ReflectionNatives::__reflect_getAnnotationBool(const std::vector<Value>& args)
    {
        const auto* v = fetchTypedParam(args, "__reflect_getAnnotationBool");
        requireType(v, AnnotationValueType::BOOL, "__reflect_getAnnotationBool", "bool");
        return v->asBool();
    }

    Value ReflectionNatives::__reflect_getAnnotationString(const std::vector<Value>& args)
    {
        const auto* v = fetchTypedParam(args, "__reflect_getAnnotationString");
        requireType(v, AnnotationValueType::STRING, "__reflect_getAnnotationString", "string");
        return v->asString();
    }

    Value ReflectionNatives::__reflect_getAnnotationClass(const std::vector<Value>& args)
    {
        const auto* v = fetchTypedParam(args, "__reflect_getAnnotationClass");
        requireType(v, AnnotationValueType::CLASS_REF, "__reflect_getAnnotationClass", "Class");
        // Return the textual class name; mType-side callers pipe this through
        // Class::forName() to obtain a Class instance. Keeps reflection.mt
        // using only public Class APIs (ctor is private).
        return v->asClassRef();
    }

    Value ReflectionNatives::__reflect_getAnnotationClassArray(const std::vector<Value>& args)
    {
        const auto* v = fetchTypedParam(args, "__reflect_getAnnotationClassArray");
        if (!v) throw errors::RuntimeException("__reflect_getAnnotationClassArray: parameter not present");
        // Accept single-class shorthand by promoting to a 1-element array.
        std::vector<std::string> names;
        if (v->getType() == AnnotationValueType::CLASS_ARRAY) names = v->asClassArray();
        else if (v->getType() == AnnotationValueType::CLASS_REF) names.push_back(v->asClassRef());
        else throw errors::RuntimeException("__reflect_getAnnotationClassArray: parameter is not Class[]");

        if (!currentEnvironment) throw errors::RuntimeException("ReflectionNatives: environment not initialized");
        auto reg = currentEnvironment->getClassRegistry();
        auto result = std::make_shared<NativeArray>(names.size(), ValueType::INT);
        for (size_t i = 0; i < names.size(); ++i)
        {
            int64_t handle = 0;
            if (reg)
            {
                if (auto def = reg->findClass(names[i]))
                {
                    handle = ReflectionHandleRegistry::instance().getOrCreateClassHandle(def);
                }
            }
            result->set(static_cast<int>(i), handle);
        }
        return result;
    }

    Value ReflectionNatives::__reflect_isAnnotationParamNull(const std::vector<Value>& args)
    {
        const auto* v = fetchTypedParam(args, "__reflect_isAnnotationParamNull");
        if (!v) return true;
        return v->isNull();
    }

    Value ReflectionNatives::__reflect_getMethodAnnotation(const std::vector<Value>& args)
    {
        if (args.size() != 2) throw errors::RuntimeException("__reflect_getMethodAnnotation requires 2 arguments");
        if (!std::holds_alternative<int64_t>(args[0]))
            throw errors::RuntimeException("__reflect_getMethodAnnotation requires methodHandle:int");
        int64_t methodHandle = std::get<int64_t>(args[0]);
        std::string name;
        if (std::holds_alternative<std::string>(args[1])) name = std::get<std::string>(args[1]);
        else if (std::holds_alternative<InternedString>(args[1])) name = std::get<InternedString>(args[1]).getString();
        else throw errors::RuntimeException("__reflect_getMethodAnnotation requires annotationName:string");

        auto& reg = ReflectionHandleRegistry::instance();
        auto info = reg.getMethod(methodHandle);
        if (!info.method) throw errors::RuntimeException("Invalid method handle");
        auto annotation = info.method->getAnnotation(name);
        if (!annotation) return std::monostate{};
        return reg.registerAnnotation(annotation, name);
    }

    Value ReflectionNatives::__reflect_hasMethodAnnotation(const std::vector<Value>& args)
    {
        if (args.size() != 2) throw errors::RuntimeException("__reflect_hasMethodAnnotation requires 2 arguments");
        if (!std::holds_alternative<int64_t>(args[0]))
            throw errors::RuntimeException("__reflect_hasMethodAnnotation requires methodHandle:int");
        int64_t methodHandle = std::get<int64_t>(args[0]);
        std::string name;
        if (std::holds_alternative<std::string>(args[1])) name = std::get<std::string>(args[1]);
        else if (std::holds_alternative<InternedString>(args[1])) name = std::get<InternedString>(args[1]).getString();
        else throw errors::RuntimeException("__reflect_hasMethodAnnotation requires annotationName:string");

        auto info = ReflectionHandleRegistry::instance().getMethod(methodHandle);
        if (!info.method) throw errors::RuntimeException("Invalid method handle");
        return info.method->hasAnnotation(name);
    }

    Value ReflectionNatives::__reflect_getFieldAnnotation(const std::vector<Value>& args)
    {
        if (args.size() != 2) throw errors::RuntimeException("__reflect_getFieldAnnotation requires 2 arguments");
        if (!std::holds_alternative<int64_t>(args[0]))
            throw errors::RuntimeException("__reflect_getFieldAnnotation requires fieldHandle:int");
        int64_t fieldHandle = std::get<int64_t>(args[0]);
        std::string name;
        if (std::holds_alternative<std::string>(args[1])) name = std::get<std::string>(args[1]);
        else if (std::holds_alternative<InternedString>(args[1])) name = std::get<InternedString>(args[1]).getString();
        else throw errors::RuntimeException("__reflect_getFieldAnnotation requires annotationName:string");

        auto& reg = ReflectionHandleRegistry::instance();
        auto info = reg.getField(fieldHandle);
        if (!info.field) throw errors::RuntimeException("Invalid field handle");
        auto annotation = info.field->getAnnotation(name);
        if (!annotation) return std::monostate{};
        return reg.registerAnnotation(annotation, name);
    }

    Value ReflectionNatives::__reflect_hasFieldAnnotation(const std::vector<Value>& args)
    {
        if (args.size() != 2) throw errors::RuntimeException("__reflect_hasFieldAnnotation requires 2 arguments");
        if (!std::holds_alternative<int64_t>(args[0]))
            throw errors::RuntimeException("__reflect_hasFieldAnnotation requires fieldHandle:int");
        int64_t fieldHandle = std::get<int64_t>(args[0]);
        std::string name;
        if (std::holds_alternative<std::string>(args[1])) name = std::get<std::string>(args[1]);
        else if (std::holds_alternative<InternedString>(args[1])) name = std::get<InternedString>(args[1]).getString();
        else throw errors::RuntimeException("__reflect_hasFieldAnnotation requires annotationName:string");

        auto info = ReflectionHandleRegistry::instance().getField(fieldHandle);
        if (!info.field) throw errors::RuntimeException("Invalid field handle");
        return info.field->hasAnnotation(name);
    }

    Value ReflectionNatives::__reflect_getConstructorAnnotations(const std::vector<Value>& args)
    {
        if (args.size() != 1) throw errors::RuntimeException("__reflect_getConstructorAnnotations requires 1 argument");
        if (!std::holds_alternative<int64_t>(args[0]))
            throw errors::RuntimeException("__reflect_getConstructorAnnotations requires ctorHandle:int");
        int64_t ctorHandle = std::get<int64_t>(args[0]);

        auto& reg = ReflectionHandleRegistry::instance();
        auto info = reg.getConstructor(ctorHandle);
        if (!info.constructor) throw errors::RuntimeException("Invalid constructor handle");

        const auto& annotations = info.constructor->getAnnotations();
        std::vector<int> handles;
        handles.reserve(annotations.size());
        for (const auto& ann : annotations)
        {
            if (!ann) continue;
            int64_t h = reg.registerAnnotation(ann, ann->getName());
            handles.push_back(static_cast<int>(h));
        }
        auto result = std::make_shared<NativeArray>(handles.size(), ValueType::INT);
        for (size_t i = 0; i < handles.size(); ++i) result->set(static_cast<int>(i), handles[i]);
        return result;
    }

    Value ReflectionNatives::__reflect_getConstructorAnnotation(const std::vector<Value>& args)
    {
        if (args.size() != 2) throw errors::RuntimeException("__reflect_getConstructorAnnotation requires 2 arguments");
        if (!std::holds_alternative<int64_t>(args[0]))
            throw errors::RuntimeException("__reflect_getConstructorAnnotation requires ctorHandle:int");
        int64_t ctorHandle = std::get<int64_t>(args[0]);
        std::string name;
        if (std::holds_alternative<std::string>(args[1])) name = std::get<std::string>(args[1]);
        else if (std::holds_alternative<InternedString>(args[1])) name = std::get<InternedString>(args[1]).getString();
        else throw errors::RuntimeException("__reflect_getConstructorAnnotation requires annotationName:string");

        auto& reg = ReflectionHandleRegistry::instance();
        auto info = reg.getConstructor(ctorHandle);
        if (!info.constructor) throw errors::RuntimeException("Invalid constructor handle");
        auto annotation = info.constructor->getAnnotation(name);
        if (!annotation) return std::monostate{};
        return reg.registerAnnotation(annotation, name);
    }

    Value ReflectionNatives::__reflect_hasConstructorAnnotation(const std::vector<Value>& args)
    {
        if (args.size() != 2) throw errors::RuntimeException("__reflect_hasConstructorAnnotation requires 2 arguments");
        if (!std::holds_alternative<int64_t>(args[0]))
            throw errors::RuntimeException("__reflect_hasConstructorAnnotation requires ctorHandle:int");
        int64_t ctorHandle = std::get<int64_t>(args[0]);
        std::string name;
        if (std::holds_alternative<std::string>(args[1])) name = std::get<std::string>(args[1]);
        else if (std::holds_alternative<InternedString>(args[1])) name = std::get<InternedString>(args[1]).getString();
        else throw errors::RuntimeException("__reflect_hasConstructorAnnotation requires annotationName:string");

        auto info = ReflectionHandleRegistry::instance().getConstructor(ctorHandle);
        if (!info.constructor) throw errors::RuntimeException("Invalid constructor handle");
        return info.constructor->hasAnnotation(name);
    }

    Value ReflectionNatives::__reflect_getAnnotationMeta(const std::vector<Value>& args)
    {
        if (args.size() != 2) throw errors::RuntimeException("__reflect_getAnnotationMeta requires 2 arguments");
        if (!std::holds_alternative<int64_t>(args[0]))
            throw errors::RuntimeException("__reflect_getAnnotationMeta requires annotationHandle:int");
        int64_t handle = std::get<int64_t>(args[0]);
        std::string metaName;
        if (std::holds_alternative<std::string>(args[1])) metaName = std::get<std::string>(args[1]);
        else if (std::holds_alternative<InternedString>(args[1])) metaName = std::get<InternedString>(args[1]).getString();
        else throw errors::RuntimeException("__reflect_getAnnotationMeta requires metaAnnotationName:string");

        auto& reg = ReflectionHandleRegistry::instance();
        auto info = reg.getAnnotation(handle);
        if (!info.annotation) return std::monostate{};
        if (!currentEnvironment) return std::monostate{};
        auto annRegistry = currentEnvironment->getAnnotationRegistry();
        if (!annRegistry) return std::monostate{};
        auto def = annRegistry->findAnnotation(info.annotationName);
        if (!def) return std::monostate{};
        auto meta = def->getMetaAnnotation(metaName);
        if (!meta) return std::monostate{};
        return reg.registerAnnotation(meta, metaName);
    }

    Value ReflectionNatives::__reflect_getAnnotationObject(const std::vector<Value>& args)
    {
        if (args.size() != 2)
            throw errors::RuntimeException("__reflect_getAnnotationObject requires (annotationHandle, annotationName)");
        if (!std::holds_alternative<int64_t>(args[0]))
            throw errors::RuntimeException("__reflect_getAnnotationObject requires annotationHandle:int");
        int64_t handle = std::get<int64_t>(args[0]);
        std::string name;
        if (std::holds_alternative<std::string>(args[1])) name = std::get<std::string>(args[1]);
        else if (std::holds_alternative<InternedString>(args[1])) name = std::get<InternedString>(args[1]).getString();
        else throw errors::RuntimeException("__reflect_getAnnotationObject requires annotationName:string");

        auto info = ReflectionHandleRegistry::instance().getAnnotation(handle);
        if (!info.annotation) return std::monostate{};
        if (!currentEnvironment) return std::monostate{};
        auto reg = currentEnvironment->getAnnotationRegistry();
        if (!reg) return std::monostate{};
        auto def = reg->findAnnotation(name);
        if (!def) return std::monostate{};
        auto instance = AnnotationInstanceFactory::buildInstance(def, info.annotation);
        if (!instance) return std::monostate{};
        return instance;
    }
}
