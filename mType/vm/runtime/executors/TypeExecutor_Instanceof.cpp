#include "TypeExecutor.hpp"
#include <cctype>
#include "../utils/NullCheckUtils.hpp"
#include "../../../value/ValueObject.hpp"
#include "../../../value/ValueShim.hpp"
#include "../../../types/TypeConversionUtils.hpp"
#include "../../../environment/registry/InterfaceDefinition.hpp"

namespace vm::runtime
{
    // ============================================================
    // MYT-44 helpers — promoted to private statics on TypeExecutor.
    // Implement string-level substitution needed to discriminate
    // `list isClassOf List<Int>` from `list isClassOf List<String>`,
    // given that ClassDefinition::implementedInterfaces stores entries
    // verbatim from the parser ("List<E>" where E references the
    // declaring class's own type parameter).
    // ============================================================

    std::string TypeExecutor::trimString(const std::string& s)
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

    // Split a type-parameter list like "<Int, Map<K, V>>" into its top-level
    // atoms. Tolerates both spacing conventions (parser emits no-space, see
    // ParserUtils.cpp:208; GenericType::toString() emits with-space). Depth
    // counter guarantees inner commas inside nested `<...>` do not split the
    // outer list. Accepts either the bracketed form ("<...>") or the bare
    // inner body. Empty/malformed input returns empty (defensive).
    std::vector<std::string> TypeExecutor::splitTopLevelTypeArgs(const std::string& params)
    {
        std::string body = trimString(params);
        if (body.empty())
        {
            return {};
        }
        if (body.front() == '<' && body.back() == '>')
        {
            body = body.substr(1, body.size() - 2);
        }
        body = trimString(body);
        if (body.empty())
        {
            return {};
        }

        std::vector<std::string> atoms;
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
                    return {};
                }
            }
            else if (c == ',' && depth == 0)
            {
                std::string piece = trimString(body.substr(argStart, i - argStart));
                if (!piece.empty())
                {
                    atoms.push_back(std::move(piece));
                }
                argStart = i + 1;
            }
        }

        if (depth != 0)
        {
            return {};
        }

        std::string last = trimString(body.substr(argStart));
        if (!last.empty())
        {
            atoms.push_back(std::move(last));
        }
        return atoms;
    }

    // Recursively substitute type-parameter names in a type expression using
    // a bindings map like {"E": "Int"}. Output always uses ", " (with space)
    // between top-level atoms so the result lines up with GenericType::
    // toString() / MYT-41's buildParameterizedName spacing contract, enabling
    // string equality against compiler-emitted target strings. Only leaf
    // atoms are looked up; "List" stays a class name, only "E" is replaced.
    // Unknown atoms left verbatim (matches buildParameterizedName fallback).
    std::string TypeExecutor::substituteTypeExpression(
        const std::string& expr,
        const std::unordered_map<std::string, std::string>& bindings)
    {
        std::string trimmed = trimString(expr);
        if (trimmed.empty())
        {
            return trimmed;
        }

        size_t genericStart = trimmed.find('<');
        std::string base;
        std::string params;
        if (genericStart == std::string::npos)
        {
            base = trimmed;
            params = "";
        }
        else
        {
            base = trimmed.substr(0, genericStart);
            params = trimmed.substr(genericStart);
        }

        if (params.empty())
        {
            auto it = bindings.find(base);
            if (it != bindings.end() && !it->second.empty())
            {
                return it->second;
            }
            return base;
        }

        std::vector<std::string> atoms = splitTopLevelTypeArgs(params);
        std::string out = base;
        out += "<";
        for (size_t i = 0; i < atoms.size(); ++i)
        {
            if (i > 0) out += ", ";
            out += substituteTypeExpression(atoms[i], bindings);
        }
        out += ">";
        return out;
    }

    // Compute fresh bindings for recursion into an interface's extended-
    // interfaces list. Rebind step makes interface-on-interface inheritance
    // correct when parameter names differ across the chain. On arity mismatch
    // (malformed input or raw extends), returns empty — callers interpret
    // that as "bail out of parameterized recursion for this branch."
    std::unordered_map<std::string, std::string> TypeExecutor::rebindForInterface(
        const runtimeTypes::klass::InterfaceDefinition* ifaceDef,
        const std::string& substitutedParams)
    {
        std::unordered_map<std::string, std::string> result;
        if (!ifaceDef)
        {
            return result;
        }

        const auto& declaredParams = ifaceDef->getGenericParameters();
        if (declaredParams.empty())
        {
            return result;
        }

        std::vector<std::string> atoms = splitTopLevelTypeArgs(substitutedParams);
        if (atoms.size() != declaredParams.size())
        {
            return result;
        }

        for (size_t i = 0; i < declaredParams.size(); ++i)
        {
            result[declaredParams[i].name] = atoms[i];
        }
        return result;
    }

    bool TypeExecutor::checkInstanceofObject(
        const value::Value& val,
        const std::string& targetTypeName,
        environment::Environment* env) {
        if (val.tag() == value::ValueType::ARRAY) {
            return checkInstanceofArray(val, targetTypeName);
        }
        // MYT-208: accept STACK_OBJECT — instanceof on a stack-promoted local
        // is a routine pattern (e.g. `if (p isClassOf Point)`).
        if (value::isAnyObject(val)) {
            return checkInstanceofObjectInstance(val, targetTypeName, env);
        }
        if (value::isValueObject(val)) {
            return checkInstanceofValueObject(val, targetTypeName, env);
        }
        return false;
    }

    bool TypeExecutor::checkInstanceofArray(
        const value::Value& val,
        const std::string& targetTypeName)
    {
        // MYT-281: arrays participate as `Object` subtypes. `arr isClassOf
        // Object` is true for any array-shaped Value; `arr isClassOf T[]...`
        // is true when both rank and element type match.
        if (targetTypeName == "Object") {
            return true;
        }
        if (value::isNativeArray(val)) {
            auto arr = value::asNativeArray(val);
            std::string fullName = arr->getElementTypeName() + "[]";
            return fullName == targetTypeName;
        }
        std::string multiDimFullName = reconstructMultiArrayTypeName(val);
        if (!multiDimFullName.empty()) {
            return multiDimFullName == targetTypeName;
        }
        return false;
    }

    bool TypeExecutor::checkInstanceofObjectInstance(
        const value::Value& val,
        const std::string& targetTypeName,
        environment::Environment* env)
    {
        auto* obj = value::asObjectInstanceRaw(val);
        if (!obj) {
            return false;
        }
        // Reconstruct FULL parameterized type ("Box<Int>") from runtime
        // bindings — classDef->getName() is the raw "Box" only (MYT-40
        // Option A). This is what enables `box isClassOf Box<Int>` to
        // discriminate from `Box<String>`.
        std::string className = reconstructObjectFullType(obj);
        return matchClassAgainstTarget(obj->getClassDefinition(), className,
                                       obj->getGenericTypeBindings(),
                                       targetTypeName, env);
    }

    bool TypeExecutor::checkInstanceofValueObject(
        const value::Value& val,
        const std::string& targetTypeName,
        environment::Environment* env)
    {
        auto obj = value::asValueObject(val);
        if (!obj) {
            return false;
        }
        // Value classes can also be generic — mirror the ObjectInstance path.
        // Most value-class instances won't have bindings, so reconstruction
        // reduces to classDef->getName() in that case.
        std::string className = reconstructValueObjectFullType(obj);
        return matchClassAgainstTarget(obj->getClassDefinition(), className,
                                       obj->getGenericTypeBindings(),
                                       targetTypeName, env);
    }

    bool TypeExecutor::matchClassAgainstTarget(
        std::shared_ptr<runtimeTypes::klass::ClassDefinition> classDef,
        const std::string& className,
        const std::unordered_map<std::string, std::string>& objBindings,
        const std::string& targetTypeName,
        environment::Environment* env)
    {
        std::string baseClassName = ::types::TypeConversionUtils::extractBaseTypeName(className);
        std::string baseTargetName = ::types::TypeConversionUtils::extractBaseTypeName(targetTypeName);

        // Parameterized-target guard: if the target has type arguments, must
        // NOT fall through to base-only match — else Box<String> would
        // incorrectly satisfy `isClassOf Box<Int>`.
        bool targetHasParams = (targetTypeName.find('<') != std::string::npos);

        bool result = false;
        if (className == targetTypeName) {
            result = true;
        } else if (!targetHasParams && baseClassName == baseTargetName) {
            // Raw target — backward compat: `Box<Int> isClassOf Box` → true.
            result = true;
        }

        if (!result) {
            if (!targetHasParams) {
                // Raw target — walk subclass chain by name, allow
                // erased-to-base matches.
                result = classDef->isSubclassOf(targetTypeName);
                if (!result && baseTargetName != targetTypeName) {
                    result = classDef->isSubclassOf(baseTargetName);
                }
            } else {
                // Parameterized target — upcast only succeeds when base
                // matches by inheritance AND type-argument strings agree.
                // Propagating type arguments through `extends` is out of
                // scope for v1.
                std::string classParams;
                size_t genericStart = className.find('<');
                if (genericStart != std::string::npos) {
                    classParams = className.substr(genericStart);
                }
                std::string targetParams = targetTypeName.substr(targetTypeName.find('<'));
                if (classDef->isSubclassOf(baseTargetName) &&
                    classParams == targetParams) {
                    result = true;
                }
            }
        }

        // Check implemented interfaces. Two paths:
        //   - Raw target (MYT-41): strip generics, compare base names, recurse
        //     via raw checkInterfaceHierarchy.
        //   - Parameterized target (MYT-44): substitute interface entries
        //     through the receiver's generic bindings, compare reconstructed
        //     form verbatim, recurse via checkInterfaceHierarchyParam with
        //     rebound bindings at each interface-inheritance step.
        if (!result) {
            auto currentClass = classDef;
            while (currentClass && !result) {
                const auto& interfaces = currentClass->getImplementedInterfaces();
                for (const auto& iface : interfaces) {
                    if (targetHasParams) {
                        std::string substituted = substituteTypeExpression(iface, objBindings);
                        if (substituted == targetTypeName) {
                            result = true;
                            break;
                        }
                        std::unordered_set<std::string> visited;
                        if (checkInterfaceHierarchyParam(
                                substituted, targetTypeName, visited,
                                objBindings, env)) {
                            result = true;
                            break;
                        }
                    } else {
                        std::string baseIfaceName =
                            ::types::TypeConversionUtils::extractBaseTypeName(iface);
                        if (iface == targetTypeName || baseIfaceName == baseTargetName) {
                            result = true;
                            break;
                        }
                        std::unordered_set<std::string> visited;
                        if (checkInterfaceHierarchy(iface, targetTypeName, visited, env)) {
                            result = true;
                            break;
                        }
                    }
                }
                currentClass = currentClass->getParentClass();
            }
        }

        return result;
    }

    TypeExecutor::TypeComponents TypeExecutor::extractTypeComponents(const std::string& typeName) {
        TypeComponents components;
        components.baseName = typeName;
        components.typeParams = "";

        size_t genericStart = typeName.find('<');
        if (genericStart != std::string::npos) {
            components.baseName = typeName.substr(0, genericStart);
            components.typeParams = typeName.substr(genericStart);
        }

        return components;
    }

    bool TypeExecutor::checkExactMatch(const std::string& className, const std::string& targetTypeName,
                                       const TypeComponents& classComp, const TypeComponents& targetComp) {
        if (className == targetTypeName) {
            return true;
        }
        // Base class match without type params (Box<Int> → Box).
        if (classComp.baseName == targetComp.baseName && targetComp.typeParams.empty()) {
            return true;
        }
        // Type erasure compatibility: erased → parameterized (Box → Box<Int>)
        // because mType uses type erasure at runtime.
        if (classComp.baseName == targetComp.baseName && classComp.typeParams.empty()) {
            return true;
        }
        return false;
    }

    bool TypeExecutor::checkUpcastMatch(std::shared_ptr<runtimeTypes::klass::ClassDefinition> classDef,
                                        const std::string& targetTypeName, const TypeComponents& targetComp,
                                        const std::string& baseClassName, const std::string& classTypeParams) {
        if (classDef->isSubclassOf(targetTypeName)) {
            return true;
        }
        // Upcast with base names (for generic types).
        if (classDef->isSubclassOf(targetComp.baseName)) {
            // Type parameters must match for generic upcast when target has them.
            if (!targetComp.typeParams.empty() && !classTypeParams.empty()) {
                return (classTypeParams == targetComp.typeParams);
            }
            return true;
        }
        return false;
    }

    bool TypeExecutor::checkDowncastMatch(const std::string& baseClassName, const std::string& baseTargetName,
                                          const std::string& classTypeParams, const std::string& targetTypeParams) {
        auto targetClass = environment->findClass(baseTargetName);
        if (!targetClass) {
            return false;
        }

        if (!targetClass->isSubclassOf(baseClassName)) {
            return false;
        }

        // Type parameters must match for valid generic downcast.
        if (!targetTypeParams.empty() && !classTypeParams.empty()) {
            return (classTypeParams == targetTypeParams);
        }
        return true;
    }

    bool TypeExecutor::checkInterfaceMatch(std::shared_ptr<runtimeTypes::klass::ClassDefinition> classDef,
                                           const std::string& targetTypeName) {
        const auto& interfaces = classDef->getImplementedInterfaces();
        for (const auto& iface : interfaces) {
            std::unordered_set<std::string> visited;
            if (checkInterfaceHierarchy(iface, targetTypeName, visited, environment.get())) {
                return true;
            }
        }
        return false;
    }
}
