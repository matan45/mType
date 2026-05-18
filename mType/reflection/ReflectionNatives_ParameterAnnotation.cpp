#include "ReflectionNatives.hpp"
#include <cstddef>
#include <cstdint>
#include "ReflectionHandle.hpp"
#include "../value/NativeArray.hpp"
#include "../value/InternedString.hpp"
#include "../errors/RuntimeException.hpp"
#include "../environment/registry/MethodDefinition.hpp"
#include "../environment/registry/ConstructorDefinition.hpp"

// MYT-110: parameter-level annotation reflection. Mirrors the method/ctor
// singular-annotation natives in ReflectionNatives_TypedAnnotation.cpp but
// threads an additional paramIndex argument and pulls data from
// {Method,Constructor}Definition::getParameterAnnotation(s).

namespace reflection
{
    using namespace value;

    namespace
    {
        int64_t extractHandle(const Value& arg, const char* fn, const char* label)
        {
            if (!isInt(arg))
                throw errors::RuntimeException(std::string(fn) + " requires " + label + ":int");
            return asInt(arg);
        }

        std::string extractName(const Value& arg, const char* fn, const char* label)
        {
            // MYT-317: SSO-aware.
            if (isAnyString(arg)) return std::string(asStringView(arg));
            throw errors::RuntimeException(std::string(fn) + " requires " + label + ":string");
        }

        // Returns the paramIndex, after bounds-checking against the container's
        // parameter count (we accept any in-range index; annotations may be
        // empty and that's fine).
        size_t requireIndex(int64_t idx, size_t size, const char* fn)
        {
            if (idx < 0 || static_cast<size_t>(idx) >= size)
                throw errors::RuntimeException(std::string(fn) + ": parameter index out of bounds");
            return static_cast<size_t>(idx);
        }
    }

    // --- Method parameter annotations ---------------------------------------

    Value ReflectionNatives::__reflect_getMethodParameterAnnotations(void* userData, environment::NativeContext& ctx, std::span<const value::Value> args)
    {
        constexpr const char* FN = "__reflect_getMethodParameterAnnotations";
        if (args.size() != 2) throw errors::RuntimeException(std::string(FN) + " requires 2 arguments");
        int64_t methodHandle = extractHandle(args[0], FN, "methodHandle");
        int64_t paramIndexI = extractHandle(args[1], FN, "paramIndex");

        auto& reg = ReflectionHandleRegistry::instance();
        auto info = reg.getMethod(methodHandle);
        if (!info.method) throw errors::RuntimeException("Invalid method handle");
        // MYT-214: paramIndex is user-facing (excludes implicit `this`). Translate
        // to the underlying parameters/parameterAnnotations vector index.
        const auto& params = info.method->getParameters();
        const size_t thisOffset = (!info.method->isStatic() && !params.empty()) ? 1 : 0;
        size_t pi = requireIndex(paramIndexI, params.size() - thisOffset, FN) + thisOffset;

        const auto& list = info.method->getParameterAnnotations(pi);
        std::vector<int64_t> handles;
        handles.reserve(list.size());
        for (const auto& ann : list)
        {
            if (!ann) continue;
            handles.push_back(reg.registerAnnotation(ann, ann->getName()));
        }
        auto result = std::make_shared<NativeArray>(handles.size(), ValueType::INT);
        for (size_t i = 0; i < handles.size(); ++i) result->set(static_cast<int>(i), handles[i]);
        return result;
    }

    Value ReflectionNatives::__reflect_getMethodParameterAnnotation(void* userData, environment::NativeContext& ctx, std::span<const value::Value> args)
    {
        constexpr const char* FN = "__reflect_getMethodParameterAnnotation";
        if (args.size() != 3) throw errors::RuntimeException(std::string(FN) + " requires 3 arguments");
        int64_t methodHandle = extractHandle(args[0], FN, "methodHandle");
        int64_t paramIndexI = extractHandle(args[1], FN, "paramIndex");
        std::string name = extractName(args[2], FN, "annotationName");

        auto& reg = ReflectionHandleRegistry::instance();
        auto info = reg.getMethod(methodHandle);
        if (!info.method) throw errors::RuntimeException("Invalid method handle");
        // MYT-214: paramIndex is user-facing; translate past implicit `this`.
        const auto& params = info.method->getParameters();
        const size_t thisOffset = (!info.method->isStatic() && !params.empty()) ? 1 : 0;
        size_t pi = requireIndex(paramIndexI, params.size() - thisOffset, FN) + thisOffset;

        auto annotation = info.method->getParameterAnnotation(pi, name);
        // Return int 0 on not-found so mType `int handle = ...` receives a real int.
        if (!annotation) return static_cast<int64_t>(0);
        return reg.registerAnnotation(annotation, name);
    }

    Value ReflectionNatives::__reflect_hasMethodParameterAnnotation(void* userData, environment::NativeContext& ctx, std::span<const value::Value> args)
    {
        constexpr const char* FN = "__reflect_hasMethodParameterAnnotation";
        if (args.size() != 3) throw errors::RuntimeException(std::string(FN) + " requires 3 arguments");
        int64_t methodHandle = extractHandle(args[0], FN, "methodHandle");
        int64_t paramIndexI = extractHandle(args[1], FN, "paramIndex");
        std::string name = extractName(args[2], FN, "annotationName");

        auto info = ReflectionHandleRegistry::instance().getMethod(methodHandle);
        if (!info.method) throw errors::RuntimeException("Invalid method handle");
        // MYT-214: paramIndex is user-facing; translate past implicit `this`.
        const auto& params = info.method->getParameters();
        const size_t thisOffset = (!info.method->isStatic() && !params.empty()) ? 1 : 0;
        size_t pi = requireIndex(paramIndexI, params.size() - thisOffset, FN) + thisOffset;
        return info.method->hasParameterAnnotation(pi, name);
    }

    // --- Constructor parameter annotations ----------------------------------

    Value ReflectionNatives::__reflect_getConstructorParameterAnnotations(void* userData, environment::NativeContext& ctx, std::span<const value::Value> args)
    {
        constexpr const char* FN = "__reflect_getConstructorParameterAnnotations";
        if (args.size() != 2) throw errors::RuntimeException(std::string(FN) + " requires 2 arguments");
        int64_t ctorHandle = extractHandle(args[0], FN, "ctorHandle");
        int64_t paramIndexI = extractHandle(args[1], FN, "paramIndex");

        auto& reg = ReflectionHandleRegistry::instance();
        auto info = reg.getConstructor(ctorHandle);
        if (!info.constructor) throw errors::RuntimeException("Invalid constructor handle");
        size_t pi = requireIndex(paramIndexI, info.constructor->getParametersWithTypes().size(), FN);

        const auto& list = info.constructor->getParameterAnnotations(pi);
        std::vector<int64_t> handles;
        handles.reserve(list.size());
        for (const auto& ann : list)
        {
            if (!ann) continue;
            handles.push_back(reg.registerAnnotation(ann, ann->getName()));
        }
        auto result = std::make_shared<NativeArray>(handles.size(), ValueType::INT);
        for (size_t i = 0; i < handles.size(); ++i) result->set(static_cast<int>(i), handles[i]);
        return result;
    }

    Value ReflectionNatives::__reflect_getConstructorParameterAnnotation(void* userData, environment::NativeContext& ctx, std::span<const value::Value> args)
    {
        constexpr const char* FN = "__reflect_getConstructorParameterAnnotation";
        if (args.size() != 3) throw errors::RuntimeException(std::string(FN) + " requires 3 arguments");
        int64_t ctorHandle = extractHandle(args[0], FN, "ctorHandle");
        int64_t paramIndexI = extractHandle(args[1], FN, "paramIndex");
        std::string name = extractName(args[2], FN, "annotationName");

        auto& reg = ReflectionHandleRegistry::instance();
        auto info = reg.getConstructor(ctorHandle);
        if (!info.constructor) throw errors::RuntimeException("Invalid constructor handle");
        size_t pi = requireIndex(paramIndexI, info.constructor->getParametersWithTypes().size(), FN);

        auto annotation = info.constructor->getParameterAnnotation(pi, name);
        if (!annotation) return static_cast<int64_t>(0);
        return reg.registerAnnotation(annotation, name);
    }

    Value ReflectionNatives::__reflect_hasConstructorParameterAnnotation(void* userData, environment::NativeContext& ctx, std::span<const value::Value> args)
    {
        constexpr const char* FN = "__reflect_hasConstructorParameterAnnotation";
        if (args.size() != 3) throw errors::RuntimeException(std::string(FN) + " requires 3 arguments");
        int64_t ctorHandle = extractHandle(args[0], FN, "ctorHandle");
        int64_t paramIndexI = extractHandle(args[1], FN, "paramIndex");
        std::string name = extractName(args[2], FN, "annotationName");

        auto info = ReflectionHandleRegistry::instance().getConstructor(ctorHandle);
        if (!info.constructor) throw errors::RuntimeException("Invalid constructor handle");
        size_t pi = requireIndex(paramIndexI, info.constructor->getParametersWithTypes().size(), FN);
        return info.constructor->hasParameterAnnotation(pi, name);
    }
}






