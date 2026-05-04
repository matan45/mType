#include "ReflectionNatives.hpp"
#include <cstdint>
#include "ReflectionHandle.hpp"
#include "../errors/RuntimeException.hpp"
#include "../runtimeTypes/klass/ObjectInstance.hpp"
#include "../value/NativeArray.hpp"
#include "../value/InternedString.hpp"
#include "../value/ObjectInstancePool.hpp"
#include "../value/ValueShim.hpp"
#include "../types/UnifiedType.hpp"
#include "../types/ReifiedTypeRegistry.hpp"
#include "../environment/registry/ClassRegistry.hpp"
#include "../vm/runtime/utils/ErrorLocationHelper.hpp"
#include <cctype>
#include <utility>

namespace reflection
{
    using namespace value;
    using namespace runtimeTypes::klass;

    namespace
    {
        // Trim ASCII whitespace from both ends.
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

        /**
         * Parse a type expression string like "Box", "Box<Int>", or
         * "Map<String, List<Int>>" into its base name and a list of argument
         * substrings (kept as full substrings so nested generics can be
         * recursively parsed).
         *
         * Tolerates whitespace around commas and inside angle brackets.
         * Throws on unbalanced brackets or empty arguments.
         */
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

            // Body is everything between the outermost '<' and the final '>'.
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

        /**
         * Recursively resolve a type expression string to a canonically-interned
         * UnifiedTypePtr via ReifiedTypeRegistry::intern.
         *
         * For each subterm, verifies that the base name resolves to a real
         * ClassDefinition in the environment, so errors surface at the precise
         * missing-type level (e.g. "Class not found: NotAType" even when nested
         * inside "Box<NotAType>").
         */
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
    } // anonymous namespace

    // ========== Class Reflection Implementations ==========

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
                // resolveToReifiedType already verifies the base is resolvable,
                // but guard defensively in case interning normalized the name.
                vm::runtime::utils::ErrorLocationHelper::throwUserException(
                    ctx.env, "ClassNotFoundException",
                    "Class not found: " + reified->getName());
            }
            int64_t handle = registry.getOrCreateClosedClassHandle(baseDef, reified);
            return static_cast<int>(handle);
        }

        // Open form: existing path.
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

    Value ReflectionNatives::__reflect_getInterfaces(void* userData, environment::NativeContext& ctx, std::span<const value::Value> args)
    {
        validateArgCount(args, 1, "__reflect_getInterfaces");
        int64_t classHandle = extractInt(args[0], "__reflect_getInterfaces", "classHandle");

        auto& registry = ReflectionHandleRegistry::instance();
        auto classDef = registry.getClass(classHandle);
        if (!classDef)
        {
            throw errors::RuntimeException("Invalid class handle");
        }

        const auto& interfaces = classDef->getImplementedInterfaces();
        auto result = std::make_shared<NativeArray>(interfaces.size(), ValueType::STRING);
        for (size_t i = 0; i < interfaces.size(); ++i)
        {
            result->set(static_cast<int>(i), interfaces[i]);
        }

        return result;
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
        for (const auto& [name, fieldDef] : classDef->getInstanceFields())
        {
            instance->setField(name, fieldDef->getValue());
        }

        return instance;
    }

    Value ReflectionNatives::__reflect_isGenericClass(void* userData, environment::NativeContext& ctx, std::span<const value::Value> args)
    {
        validateArgCount(args, 1, "__reflect_isGenericClass");
        int64_t classHandle = extractInt(args[0], "__reflect_isGenericClass", "classHandle");

        auto& registry = ReflectionHandleRegistry::instance();
        auto classDef = registry.getClass(classHandle);
        if (!classDef)
        {
            throw errors::RuntimeException("Invalid class handle");
        }

        return classDef->isGeneric();
    }

    Value ReflectionNatives::__reflect_getTypeParameters(void* userData, environment::NativeContext& ctx, std::span<const value::Value> args)
    {
        validateArgCount(args, 1, "__reflect_getTypeParameters");
        int64_t classHandle = extractInt(args[0], "__reflect_getTypeParameters", "classHandle");

        auto& registry = ReflectionHandleRegistry::instance();
        auto classDef = registry.getClass(classHandle);
        if (!classDef)
        {
            throw errors::RuntimeException("Invalid class handle");
        }

        const auto& params = classDef->getGenericParameters();
        auto result = std::make_shared<NativeArray>(params.size(), ValueType::STRING);

        for (size_t i = 0; i < params.size(); ++i)
        {
            result->set(i, params[i].name);
        }

        return result;
    }

    Value ReflectionNatives::__reflect_getTypeArguments(void* userData, environment::NativeContext& ctx, std::span<const value::Value> args)
    {
        validateArgCount(args, 1, "__reflect_getTypeArguments");
        int64_t classHandle = extractInt(args[0], "__reflect_getTypeArguments", "classHandle");

        auto& registry = ReflectionHandleRegistry::instance();
        auto classDef = registry.getClass(classHandle);
        if (!classDef)
        {
            throw errors::RuntimeException("Invalid class handle");
        }

        // Open template handles and non-generic classes return an empty array.
        auto reified = registry.getReifiedType(classHandle);
        if (!reified || reified->getTypeArguments().empty())
        {
            return std::make_shared<NativeArray>(0, ValueType::INT);
        }

        const auto& typeArgs = reified->getTypeArguments();
        auto result = std::make_shared<NativeArray>(typeArgs.size(), ValueType::INT);

        for (size_t i = 0; i < typeArgs.size(); ++i)
        {
            const auto& arg = typeArgs[i];
            if (!arg)
            {
                throw errors::RuntimeException("Invalid null type argument in reified type");
            }

            auto argBaseDef = ctx.env->findClass(arg->getName());
            if (!argBaseDef)
            {
                throw errors::RuntimeException("Class not found: " + arg->getName());
            }

            int64_t argHandle;
            if (arg->isParameterized())
            {
                // Nested closed form (e.g. List<Int> inside Map<String,List<Int>>).
                // Mint a closed handle so the caller can recursively introspect.
                argHandle = registry.getOrCreateClosedClassHandle(argBaseDef, arg);
            }
            else
            {
                argHandle = registry.getOrCreateClassHandle(argBaseDef);
            }
            result->set(static_cast<int>(i), argHandle);
        }

        return result;
    }

    Value ReflectionNatives::__reflect_getClassModifiers(void* userData, environment::NativeContext& ctx, std::span<const value::Value> args)
    {
        validateArgCount(args, 1, "__reflect_getClassModifiers");
        int64_t classHandle = extractInt(args[0], "__reflect_getClassModifiers", "classHandle");

        auto& registry = ReflectionHandleRegistry::instance();
        auto classDef = registry.getClass(classHandle);
        if (!classDef)
        {
            throw errors::RuntimeException("Invalid class handle");
        }

        return classDef->getModifierFlags();
    }

    Value ReflectionNatives::__reflect_getName(void* userData, environment::NativeContext& ctx, std::span<const value::Value> args)
    {
        validateArgCount(args, 1, "__reflect_getName");
        int64_t classHandle = extractInt(args[0], "__reflect_getName", "classHandle");

        auto& registry = ReflectionHandleRegistry::instance();
        auto classDef = registry.getClass(classHandle);
        if (!classDef)
        {
            throw errors::RuntimeException("Invalid class handle");
        }

        // Closed-form handles render the parameterized canonical string
        // (e.g. "Box<Int>"); open-form handles return the raw class name.
        auto reified = registry.getReifiedType(classHandle);
        if (reified)
        {
            return reified->toCanonicalString();
        }
        return classDef->getName();
    }

    Value ReflectionNatives::__reflect_getRawName(void* userData, environment::NativeContext& ctx, std::span<const value::Value> args)
    {
        validateArgCount(args, 1, "__reflect_getRawName");
        int64_t classHandle = extractInt(args[0], "__reflect_getRawName", "classHandle");

        auto& registry = ReflectionHandleRegistry::instance();
        auto classDef = registry.getClass(classHandle);
        if (!classDef)
        {
            throw errors::RuntimeException("Invalid class handle");
        }

        // Option A: raw name ALWAYS comes from the ClassDefinition, never
        // rendered with type arguments, matching ValueObject::getClassName().
        return classDef->getName();
    }

} // namespace reflection

