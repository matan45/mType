#include "ReflectionNatives.hpp"
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <utility>
#include "ReflectionHandle.hpp"
#include "../environment/registry/ClassRegistry.hpp"
#include "../errors/RuntimeException.hpp"
#include "../types/ReifiedTypeRegistry.hpp"
#include "../types/UnifiedType.hpp"
#include "../value/InternedString.hpp"
#include "../value/NativeArray.hpp"
#include "../value/ObjectInstance.hpp"
#include "../value/ObjectInstancePool.hpp"
#include "../value/ValueShim.hpp"
#include "../vm/runtime/utils/ErrorLocationHelper.hpp"

namespace reflection
{
    using namespace value;
    using namespace runtimeTypes::klass;

    namespace
    {
        std::string trim(const std::string& s)
        {
            size_t start = 0;
            while (start < s.size() && std::isspace(static_cast<unsigned char>(s[start])))
            {
                ++start;
            }
            size_t end = s.size();
            while (end > start && std::isspace(static_cast<unsigned char>(s[end - 1])))
            {
                --end;
            }
            return s.substr(start, end - start);
        }

        // Parse a type expression like "Box", "Box<Int>", or
        // "Map<String, List<Int>>" into its base name and a list of argument
        // substrings (full substrings so nested generics can be recursively
        // parsed). Tolerates whitespace; throws on unbalanced brackets or empty
        // arguments.
        std::pair<std::string, std::vector<std::string>> parseTypeNameArgs(const std::string& input)
        {
            std::string s = trim(input);
            if (s.empty())
            {
                throw errors::RuntimeException("Invalid type name: empty string");
            }

            size_t ltPos = s.find('<');
            if (ltPos == std::string::npos)
            {
                return {s, {}};
            }

            std::string base = trim(s.substr(0, ltPos));
            if (base.empty())
            {
                throw errors::RuntimeException("Invalid type name: missing base name in '" + input + "'");
            }

            if (s.back() != '>')
            {
                throw errors::RuntimeException("Invalid type name: unbalanced angle brackets in '" + input + "'");
            }

            std::string body = s.substr(ltPos + 1, s.size() - ltPos - 2);

            std::vector<std::string> args;
            int depth = 0;
            size_t argStart = 0;
            for (size_t i = 0; i < body.size(); ++i)
            {
                char c = body[i];
                if (c == '<')
                {
                    ++depth;
                }
                else if (c == '>')
                {
                    --depth;
                    if (depth < 0)
                    {
                        throw errors::RuntimeException(
                            "Invalid type name: unbalanced angle brackets in '" + input + "'");
                    }
                }
                else if (c == ',' && depth == 0)
                {
                    std::string piece = trim(body.substr(argStart, i - argStart));
                    if (piece.empty())
                    {
                        throw errors::RuntimeException(
                            "Invalid type name: empty type argument in '" + input + "'");
                    }
                    args.push_back(std::move(piece));
                    argStart = i + 1;
                }
            }

            if (depth != 0)
            {
                throw errors::RuntimeException(
                    "Invalid type name: unbalanced angle brackets in '" + input + "'");
            }

            std::string lastPiece = trim(body.substr(argStart));
            if (lastPiece.empty())
            {
                throw errors::RuntimeException(
                    "Invalid type name: empty type argument in '" + input + "'");
            }
            args.push_back(std::move(lastPiece));

            return {std::move(base), std::move(args)};
        }

        // Recursively resolve a type expression string to a canonically-
        // interned UnifiedTypePtr. Verifies each subterm's base resolves so
        // errors surface at the precise missing-type level (e.g. "Class not
        // found: NotAType" even when nested inside "Box<NotAType>").
        ::types::UnifiedTypePtr resolveToReifiedType(
            const std::string& typeExpr,
            const std::shared_ptr<environment::Environment>& env)
        {
            auto [baseName, argStrings] = parseTypeNameArgs(typeExpr);

            auto baseDef = env->findClass(baseName);
            if (!baseDef)
            {
                vm::runtime::utils::ErrorLocationHelper::throwUserException(
                    env, "ClassNotFoundException",
                    "Class not found: " + baseName);
            }

            std::vector<::types::UnifiedTypePtr> argTypes;
            argTypes.reserve(argStrings.size());
            for (const auto& argStr : argStrings)
            {
                argTypes.push_back(resolveToReifiedType(argStr, env));
            }

            auto type = ::types::UnifiedType::classType(baseName, std::move(argTypes));
            return env->getClassRegistry()->getReifiedTypeRegistry()->intern(type);
        }
    }

    Value ReflectionNatives::__reflect_forName(void* userData, environment::NativeContext& ctx, std::span<const value::Value> args)
    {
        validateArgCount(args, 1, "__reflect_forName");
        std::string className = extractString(args[0], "__reflect_forName", "className");

        if (!ctx.env)
        {
            throw errors::RuntimeException("Reflection environment not initialized");
        }

        auto& registry = ReflectionHandleRegistry::instance();

        // Closed (parameterized) form: e.g. "Box<Int>" or "Map<String, List<Int>>".
        // Parse, recursively resolve each type argument, intern through the
        // ReifiedTypeRegistry, and mint a closed handle distinct from the open
        // "Box" handle.
        if (className.find('<') != std::string::npos)
        {
            auto reified = resolveToReifiedType(className, ctx.env);
            auto baseDef = ctx.env->findClass(reified->getName());
            if (!baseDef)
            {
                // Defensive: resolveToReifiedType already verifies, but
                // interning could normalize the name.
                vm::runtime::utils::ErrorLocationHelper::throwUserException(
                    ctx.env, "ClassNotFoundException",
                    "Class not found: " + reified->getName());
            }
            int64_t handle = registry.getOrCreateClosedClassHandle(baseDef, reified);
            return static_cast<int>(handle);
        }

        auto classDef = ctx.env->findClass(className);
        if (!classDef)
        {
            vm::runtime::utils::ErrorLocationHelper::throwUserException(
                ctx.env, "ClassNotFoundException",
                "Class not found: " + className);
        }

        int64_t handle = registry.getOrCreateClassHandle(classDef);
        return static_cast<int>(handle);
    }

    Value ReflectionNatives::__reflect_getSimpleName(void* userData, environment::NativeContext& ctx, std::span<const value::Value> args)
    {
        validateArgCount(args, 1, "__reflect_getSimpleName");
        int64_t classHandle = extractInt(args[0], "__reflect_getSimpleName", "classHandle");

        auto& registry = ReflectionHandleRegistry::instance();
        auto classDef = registry.getClass(classHandle);
        if (!classDef)
        {
            throw errors::RuntimeException("Invalid class handle");
        }

        return classDef->getName();
    }

    Value ReflectionNatives::__reflect_getSuperclass(void* userData, environment::NativeContext& ctx, std::span<const value::Value> args)
    {
        validateArgCount(args, 1, "__reflect_getSuperclass");
        int64_t classHandle = extractInt(args[0], "__reflect_getSuperclass", "classHandle");

        auto& registry = ReflectionHandleRegistry::instance();
        auto classDef = registry.getClass(classHandle);
        if (!classDef)
        {
            throw errors::RuntimeException("Invalid class handle");
        }

        auto parentClass = classDef->getParentClass();
        if (!parentClass)
        {
            return 0;
        }

        int64_t parentHandle = registry.getOrCreateClassHandle(parentClass);
        return static_cast<int>(parentHandle);
    }

    Value ReflectionNatives::__reflect_isInterface(void* userData, environment::NativeContext& ctx, std::span<const value::Value> args)
    {
        validateArgCount(args, 1, "__reflect_isInterface");
        return false;
    }

    Value ReflectionNatives::__reflect_isAbstract(void* userData, environment::NativeContext& ctx, std::span<const value::Value> args)
    {
        validateArgCount(args, 1, "__reflect_isAbstract");
        int64_t classHandle = extractInt(args[0], "__reflect_isAbstract", "classHandle");

        auto& registry = ReflectionHandleRegistry::instance();
        auto classDef = registry.getClass(classHandle);
        if (!classDef)
        {
            throw errors::RuntimeException("Invalid class handle");
        }

        return classDef->isAbstract();
    }

    Value ReflectionNatives::__reflect_isFinal(void* userData, environment::NativeContext& ctx, std::span<const value::Value> args)
    {
        validateArgCount(args, 1, "__reflect_isFinal");
        int64_t classHandle = extractInt(args[0], "__reflect_isFinal", "classHandle");

        auto& registry = ReflectionHandleRegistry::instance();
        auto classDef = registry.getClass(classHandle);
        if (!classDef)
        {
            throw errors::RuntimeException("Invalid class handle");
        }

        return classDef->isFinal();
    }

    Value ReflectionNatives::__reflect_isInstance(void* userData, environment::NativeContext& ctx, std::span<const value::Value> args)
    {
        validateArgCount(args, 2, "__reflect_isInstance");
        int64_t classHandle = extractInt(args[0], "__reflect_isInstance", "classHandle");

        if (isVoid(args[1]) || isNullType(args[1]))
        {
            return false;
        }

        if (!isObject(args[1]))
        {
            return false;
        }

        auto& registry = ReflectionHandleRegistry::instance();
        auto classDef = registry.getClass(classHandle);
        if (!classDef)
        {
            throw errors::RuntimeException("Invalid class handle");
        }

        auto instance = asObject(args[1]);
        return instance->isInstanceOf(classDef->getName());
    }

    Value ReflectionNatives::__reflect_isAssignableFrom(void* userData, environment::NativeContext& ctx, std::span<const value::Value> args)
    {
        validateArgCount(args, 2, "__reflect_isAssignableFrom");
        int64_t thisHandle = extractInt(args[0], "__reflect_isAssignableFrom", "thisClassHandle");
        int64_t otherHandle = extractInt(args[1], "__reflect_isAssignableFrom", "otherClassHandle");

        auto& registry = ReflectionHandleRegistry::instance();
        auto thisClass = registry.getClass(thisHandle);
        auto otherClass = registry.getClass(otherHandle);

        if (!thisClass || !otherClass)
        {
            throw errors::RuntimeException("Invalid class handle");
        }

        if (thisClass->getName() == otherClass->getName())
        {
            return true;
        }

        return otherClass->isSubclassOf(thisClass->getName());
    }

    Value ReflectionNatives::__reflect_newInstance(void* userData, environment::NativeContext& ctx, std::span<const value::Value> args)
    {
        validateArgCount(args, 1, "__reflect_newInstance");
        int64_t classHandle = extractInt(args[0], "__reflect_newInstance", "classHandle");

        auto& registry = ReflectionHandleRegistry::instance();
        auto classDef = registry.getClass(classHandle);
        if (!classDef)
        {
            throw errors::RuntimeException("Invalid class handle");
        }

        if (classDef->isAbstract())
        {
            throw errors::RuntimeException("Cannot instantiate abstract class: " + classDef->getName());
        }

        auto instance = value::ObjectInstancePool::getInstance().acquire(classDef);
        const auto& instanceFields = classDef->getInstanceFields();
        for (const auto& name : classDef->getInstanceFieldOrder())
        {
            auto it = instanceFields.find(name);
            if (it == instanceFields.end()) continue;
            const auto& fieldDef = it->second;
            instance->setField(name, fieldDef->getValue());
        }

        return instance;
    }
}
