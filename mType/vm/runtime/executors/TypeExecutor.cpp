#include "TypeExecutor.hpp"
#include <cstddef>
#include <cstdint>
#include "../utils/ErrorLocationHelper.hpp"
#include "../utils/NullCheckUtils.hpp"
#include "../utils/TypeArgResolution.hpp"
#include "../../../value/StringPool.hpp"
#include "../../../value/ValueObject.hpp"
#include "../../../value/ValueShim.hpp"
#include "../../../types/TypeConversionUtils.hpp"
#include "../../../errors/UserException.hpp"
#include "../../../runtimeTypes/klass/InterfaceDefinition.hpp"
#include <sstream>
#include <iostream>
#include <cctype>

namespace vm::runtime
{
    namespace
    {
        // ==========================================================================
        // MYT-44 — file-local helpers for parameterized-interface matching.
        //
        // These implement the string-level substitution needed to discriminate
        // `list isClassOf List<Int>` from `list isClassOf List<String>`, given
        // that ClassDefinition::implementedInterfaces stores entries verbatim
        // from the parser (e.g., "List<E>" where E references the declaring
        // class's own type parameter). Nothing is wired yet; the call sites
        // come in later steps of MYT-44.
        // ==========================================================================

        // Trim ASCII whitespace from both ends of a string.
        std::string trimString(const std::string& s)
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

        // Split a type-parameter list like "<Int, Map<K, V>>" or "<Int,Map<K,V>>"
        // into its top-level atoms: ["Int", "Map<K, V>"] / ["Int", "Map<K,V>"].
        //
        // Tolerates both spacing conventions — the parser emits no-space (see
        // ParserUtils.cpp:208) while GenericType::toString() emits with-space.
        // The depth counter guarantees that inner commas (inside nested
        // angle brackets) do not split the outer list.
        //
        // Input may be the params suffix of extractTypeComponents ("<...>") or
        // the bare inner body ("..." without enclosing brackets). Both shapes
        // are handled — we strip a leading '<' and trailing '>' if present.
        // An empty or malformed input returns an empty vector (defensive).
        std::vector<std::string> splitTopLevelTypeArgs(const std::string& params)
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
                        // Unbalanced — bail out defensively.
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
                // Unbalanced — bail out defensively.
                return {};
            }

            std::string last = trimString(body.substr(argStart));
            if (!last.empty())
            {
                atoms.push_back(std::move(last));
            }
            return atoms;
        }

        // Recursively substitute type-parameter names in a type expression,
        // using a bindings map like {"E": "Int"}.
        //
        // Examples:
        //   "List<E>"            + {"E": "Int"}         → "List<Int>"
        //   "Map<K, V>"          + {"K": "S", "V": "I"} → "Map<S, I>"
        //   "List<Wrapper<E>>"   + {"E": "Int"}         → "List<Wrapper<Int>>"
        //   "List<Int>"          + {}                   → "List<Int>" (no-op)
        //   "E"                  + {"E": "Int"}         → "Int"        (leaf substitution)
        //   "E"                  + {}                   → "E"          (verbatim fallback)
        //
        // The output always uses ", " (with space) between top-level atoms so
        // the result lines up with GenericType::toString() / MYT-41's
        // buildParameterizedName spacing contract, enabling string equality
        // against compiler-emitted target strings.
        //
        // Only leaf atoms (bare identifiers with no `<...>`) are looked up in
        // bindings. The base name of a parameterized expression like
        // "List<E>" is NOT substituted — "List" stays as a class name,
        // only "E" becomes the lookup target. Unknown atoms are left
        // verbatim (matching the partial-binding fallback used by
        // buildParameterizedName at line 661).
        std::string substituteTypeExpression(
            const std::string& expr,
            const std::unordered_map<std::string, std::string>& bindings)
        {
            std::string trimmed = trimString(expr);
            if (trimmed.empty())
            {
                return trimmed;
            }

            // Split base from params using the same extractor the rest of
            // TypeExecutor already relies on (first '<').
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
                params = trimmed.substr(genericStart); // keeps the "<...>"
            }

            if (params.empty())
            {
                // Leaf — look up in bindings, otherwise verbatim fallback.
                auto it = bindings.find(base);
                if (it != bindings.end() && !it->second.empty())
                {
                    return it->second;
                }
                return base;
            }

            // Parameterized — recurse into each atom. Base name is preserved.
            std::vector<std::string> atoms = splitTopLevelTypeArgs(params);
            std::string out = base;
            out += "<";
            for (size_t i = 0; i < atoms.size(); ++i)
            {
                if (i > 0) out += ", "; // MYT-41 spacing contract
                out += substituteTypeExpression(atoms[i], bindings);
            }
            out += ">";
            return out;
        }

        // Compute a fresh binding map for recursion into an interface's own
        // extended-interfaces list. Takes the substituted type-arguments
        // (e.g., "<Int>") and the target interface's declared parameters
        // (e.g., [B] for `interface SortedList<B>`) and produces a map keyed
        // by the NEW interface's parameter names.
        //
        // This is the rebind step that makes interface-on-interface
        // inheritance correct even when parameter names differ across the
        // chain. Without it, passing parent bindings down would only work
        // when names accidentally coincide.
        //
        // On size mismatch (malformed input or raw extends), returns an
        // empty map — callers should interpret that as "bail out of
        // parameterized recursion for this branch."
        std::unordered_map<std::string, std::string> rebindForInterface(
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
                // Arity mismatch — refuse to guess, return empty.
                return result;
            }

            for (size_t i = 0; i < declaredParams.size(); ++i)
            {
                result[declaredParams[i].name] = atoms[i];
            }
            return result;
        }
    } // anonymous namespace

    TypeExecutor::TypeExecutor(ExecutionContext& ctx,
                               std::shared_ptr<environment::Environment> env)
        : context(ctx)
        , environment(std::move(env))
    {}

    // MYT-320: handleInstanceof / handleInstanceofTypeParam moved to TypeExecutor.hpp.

    bool TypeExecutor::checkInstanceOfByName(
        const value::Value& val,
        const std::string& targetTypeName,
        const std::shared_ptr<environment::Environment>& env) {
        if (checkInstanceofPrimitive(val, targetTypeName)) {
            return true;
        }
        return checkInstanceofObject(val, targetTypeName, env.get());
    }

    std::string TypeExecutor::resolveTypeParameter(const std::string& paramName) {
        // Walk from the innermost frame outward. The per-frame precedence
        // (typeArgBindings, then receiver class-level bindings) lives in
        // utils::resolveTypeParamInFrame so the JIT helpers can share it.
        auto& callStack = context.callStack;
        if (callStack.empty()) {
            throw errors::RuntimeException(
                "Type parameter '" + paramName +
                "' cannot be resolved outside of a function call");
        }

        for (auto it = callStack.rbegin(); it != callStack.rend(); ++it) {
            if (const auto* resolved = utils::resolveTypeParamInFrame(*it, paramName)) {
                return *resolved;
            }
        }

        throw errors::RuntimeException(
            "Type parameter '" + paramName + "' is not bound in this context");
    }

    void TypeExecutor::handleBindTypeArgs(const bytecode::BytecodeProgram::Instruction& instr) {
        // MYT-228: stage type-arg bindings for the next CALL_*. Operand layout:
        //   operands[0] = n (pair count)
        //   operands[1 + 3*i + 0] = paramName constant-pool index
        //   operands[1 + 3*i + 1] = TypeArgValueKind (Concrete / ForwardFromCaller)
        //   operands[1 + 3*i + 2] = value constant-pool index
        // Forward-from-caller is resolved against the CURRENT top frame's
        // bindings (we haven't pushed the new frame yet). Storing concrete
        // names means the resolver doesn't need a fixpoint walk later.
        if (instr.numOperands() == 0) {
            throw errors::RuntimeException("BIND_TYPE_ARGS: missing operand count");
        }

        // Per-IP fast path: when every binding at this site is Concrete the
        // resolved (paramName,resolved) pairs are stable. Snapshot them on
        // first execution and bulk-copy thereafter — skips N const-pool
        // indexes + N kind-branch + N rawValue construction per call.
        const size_t bindIp = context.instructionPointer;
        if (auto* cached = context.findCachedState(bindIp);
            cached && cached->cachedTypeArgBindingsValid)
        {
            auto& staged = context.pendingTypeArgs.acquireFresh();
            for (const auto& [paramName, resolved] : cached->cachedTypeArgBindings) {
                staged.emplace(paramName, resolved);
            }
            return;
        }

        const auto& constantPool = context.program->getConstantPool();
        const size_t n = static_cast<size_t>(instr.inlineOperands[0]);
        // Variable-arity guard — well-formed bytecode satisfies this; a
        // malformed .mtc would otherwise read past the operand vector.
        if (instr.numOperands() < 1 + 3 * n) {
            throw errors::RuntimeException("BIND_TYPE_ARGS: malformed operands");
        }

        // Acquire from the pool — recycles the unordered_map across calls.
        auto& staged = context.pendingTypeArgs.acquireFresh();

        bool allConcrete = true;
        std::vector<std::pair<std::string, std::string>> snapshot;
        snapshot.reserve(n);

        for (size_t i = 0; i < n; ++i) {
            const size_t base = 1 + 3 * i;
            const std::string& paramName = constantPool.getString(
                static_cast<uint32_t>(instr.operandAt(base + 0)));
            const auto kind = static_cast<bytecode::TypeArgValueKind>(
                static_cast<uint8_t>(instr.operandAt(base + 1)));
            const std::string& rawValue = constantPool.getString(
                static_cast<uint32_t>(instr.operandAt(base + 2)));

            std::string resolved;
            if (kind == bytecode::TypeArgValueKind::ForwardFromCaller) {
                // Forward-from-caller depends on the caller's frame state and
                // can't be cached at this IP. Mark the site uncacheable.
                allConcrete = false;
                if (!context.callStack.empty()) {
                    if (const auto* p = utils::resolveTypeParamInFrame(
                            context.callStack.back(), rawValue)) {
                        resolved = *p;
                    } else {
                        resolved = rawValue;
                    }
                } else {
                    resolved = rawValue;
                }
            } else {
                resolved = rawValue;
                if (allConcrete) {
                    snapshot.emplace_back(paramName, resolved);
                }
            }

            staged.emplace(paramName, std::move(resolved));
        }

        if (allConcrete) {
            auto& slot = context.getOrCreateCachedState(bindIp);
            slot.cachedTypeArgBindings = std::move(snapshot);
            slot.cachedTypeArgBindingsValid = true;
        }
    }

    void TypeExecutor::handleCastTypeParam(const bytecode::BytecodeProgram::Instruction& instr) {
        const std::string& paramName = context.program->getConstantPool().getString(instr.inlineOperands[0]);

        try {
            std::string resolved = resolveTypeParameter(paramName);
            value::Value val = context.stackManager->pop();

            if (resolved == "Int" || resolved == "int") {
                context.stackManager->push(castToInt(val));
            } else if (resolved == "Float" || resolved == "float") {
                context.stackManager->push(castToFloat(val));
            } else if (resolved == "String" || resolved == "string") {
                context.stackManager->push(castToString(val));
            } else if (resolved == "Bool" || resolved == "bool") {
                context.stackManager->push(castToBool(val));
            } else {
                context.stackManager->push(castToObject(val, resolved));
            }
        } catch (const errors::RuntimeException&) {
            // Phase 1 generics: free generic functions don't have reified type parameters at runtime.
            // In this case, the type parameter acts as erased (Object).
            // A cast to an erased type parameter is a no-op at runtime.
            // The value to be cast is already on top of the stack, so we just leave it.
        }
    }

    void TypeExecutor::handleCast(const bytecode::BytecodeProgram::Instruction& instr) {
        // Get target type name from constant pool
        const std::string& targetTypeName = context.program->getConstantPool().getString(instr.inlineOperands[0]);

        // Pop value to cast
        value::Value val = context.stackManager->pop();

        // MYT-281: array values flow through Object slots and back. `(Object)arr`
        // is an identity cast; `(T[])obj` is identity when the receiver is the
        // matching array (preserves NativeArray bridge), or an error when the
        // narrow element type disagrees. Never funnel an array through
        // castToObject — that would try to materialize an ObjectInstance.
        if (val.tag() == value::ValueType::ARRAY) {
            if (targetTypeName == "Object") {
                context.stackManager->push(val);
                return;
            }
            // Target is an array type (`T[]`, `T[][]`, ...) — identity cast
            // when the runtime element type matches. NativeArray exposes the
            // element name; multi-dim variants only validate against `Object`
            // in v1 and pass through for matching array-shaped targets.
            const bool targetIsArrayType =
                targetTypeName.size() >= 2 &&
                targetTypeName.compare(targetTypeName.size() - 2, 2, "[]") == 0;
            if (targetIsArrayType) {
                if (value::isNativeArray(val)) {
                    auto arr = value::asNativeArray(val);
                    std::string fullName = arr->getElementTypeName() + "[]";
                    if (fullName == targetTypeName) {
                        context.stackManager->push(val);
                        return;
                    }
                    utils::ErrorLocationHelper::throwUserException(context, environment,
                        "ClassCastException",
                        "cannot cast " + fullName + " to " + targetTypeName);
                }
                // MYT-281: multi-dim primitive arrays carry their element type
                // in defaultValue_'s tag (set by ArrayExecutor::buildMultiArray)
                // and rank in dimensions_. Reconstruct the full name and match
                // strictly — both rank and element type must agree. Sub-views
                // inherit defaultValue_ and have decremented rank, so a sub-
                // array of `int[][][]` correctly matches `int[][]`.
                std::string multiDimFullName = reconstructMultiArrayTypeName(val);
                if (!multiDimFullName.empty()) {
                    if (multiDimFullName == targetTypeName) {
                        context.stackManager->push(val);
                        return;
                    }
                    utils::ErrorLocationHelper::throwUserException(context, environment,
                        "ClassCastException",
                        "cannot cast " + multiDimFullName + " to " + targetTypeName);
                }
                // Defaults we can't classify (e.g. monostate-defaulted arrays
                // not produced by the primitive specialization) keep the prior
                // pass-through. No callers in the current tree create them
                // through buildMultiArray, but the safety net costs nothing.
                context.stackManager->push(val);
                return;
            }
            utils::ErrorLocationHelper::throwUserException(context, environment,
                "ClassCastException",
                "cannot cast array to " + targetTypeName);
        }

        // Perform casting based on target type
        if (targetTypeName == "Int" || targetTypeName == "int") {
            context.stackManager->push(castToInt(val));
        }
        else if (targetTypeName == "Float" || targetTypeName == "float") {
            context.stackManager->push(castToFloat(val));
        }
        else if (targetTypeName == "String" || targetTypeName == "string") {
            context.stackManager->push(castToString(val));
        }
        else if (targetTypeName == "Bool" || targetTypeName == "bool") {
            context.stackManager->push(castToBool(val));
        }
        else {
            context.stackManager->push(castToObject(val, targetTypeName));
        }
    }

    // static
    bool TypeExecutor::checkInstanceofPrimitive(const value::Value& val, const std::string& targetTypeName) {
        if (targetTypeName == "Int" || targetTypeName == "int") {
            return value::isInt(val);
        }
        if (targetTypeName == "Float" || targetTypeName == "float") {
            return value::isFloat(val);
        }
        if (targetTypeName == "Bool" || targetTypeName == "bool") {
            return value::isBool(val);
        }
        if (targetTypeName == "String" || targetTypeName == "string") {
            // MYT-317: STRING_INLINE matches the String type check.
            return value::isAnyString(val);
        }
        return false;
    }

    bool TypeExecutor::checkInstanceofObject(
        const value::Value& val,
        const std::string& targetTypeName,
        environment::Environment* env) {
        // MYT-281: arrays participate as `Object` subtypes. `arr isClassOf Object`
        // is true for any array-shaped Value; `arr isClassOf T[]...` is true when
        // both rank and element type match. NativeArray exposes the element
        // name directly; multi-dim variants reconstruct it from defaultValue_'s
        // tag plus rank.
        if (val.tag() == value::ValueType::ARRAY) {
            if (targetTypeName == "Object") {
                return true;
            }
            if (value::isNativeArray(val)) {
                auto arr = value::asNativeArray(val);
                std::string fullName = arr->getElementTypeName() + "[]";
                if (fullName == targetTypeName) {
                    return true;
                }
                return false;
            }
            std::string multiDimFullName = reconstructMultiArrayTypeName(val);
            if (!multiDimFullName.empty()) {
                return multiDimFullName == targetTypeName;
            }
            return false;
        }

        // Object type check.
        // MYT-208: accept STACK_OBJECT — instanceof on a stack-promoted local
        // is a routine pattern (e.g. `if (p isClassOf Point)`).
        if (value::isAnyObject(val)) {
            auto* obj = value::asObjectInstanceRaw(val);

            if (obj) {
                auto classDef = obj->getClassDefinition();
                // Reconstruct the FULL parameterized type (e.g. "Box<Int>") from
                // runtime bindings — classDef->getName() is the raw "Box" only
                // (MYT-40 Option A). This is what enables `box isClassOf Box<Int>`
                // to discriminate from `Box<String>`.
                std::string className = reconstructObjectFullType(obj);

                // Extract base class name from generic instantiation (e.g., "Box<Int>" -> "Box")
                std::string baseClassName = ::types::TypeConversionUtils::extractBaseTypeName(className);
                std::string baseTargetName = ::types::TypeConversionUtils::extractBaseTypeName(targetTypeName);

                // Parameterized-target guard: if the target has type arguments,
                // we must NOT fall through to the base-only match — otherwise
                // Box<String> would incorrectly satisfy `isClassOf Box<Int>`.
                // Extract the target's type-params suffix once so both the
                // exact-match and upcast paths can inspect it.
                bool targetHasParams = (targetTypeName.find('<') != std::string::npos);

                bool result = false;
                if (className == targetTypeName) {
                    // Exact full-type match — works for both raw and parameterized.
                    result = true;
                } else if (!targetHasParams && baseClassName == baseTargetName) {
                    // Raw target — backward compat: `Box<Int> isClassOf Box` → true.
                    // Only fires when the target has no `<...>`.
                    result = true;
                }

                // If not exact match, check inheritance hierarchy
                if (!result) {
                    if (!targetHasParams) {
                        // Raw target — existing behavior: walk subclass chain
                        // by name, allowing erased-to-base matches.
                        result = classDef->isSubclassOf(targetTypeName);

                        if (!result && baseTargetName != targetTypeName) {
                            result = classDef->isSubclassOf(baseTargetName);
                        }
                    } else {
                        // Parameterized target — upcast only succeeds when the
                        // base matches by inheritance AND the type argument
                        // strings agree. Propagating type arguments through
                        // `extends` is out of scope for v1 (documented).
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

                // Also check if object implements an interface with that name.
                // Walks up the inheritance chain checking interfaces at each
                // level. Two code paths coexist:
                //
                //  - Raw target (MYT-41 unchanged): strip generics, compare
                //    base names, recurse via raw checkInterfaceHierarchy.
                //  - Parameterized target (MYT-44): substitute the interface
                //    entries through the receiver's generic bindings, compare
                //    the reconstructed form verbatim, recurse via
                //    checkInterfaceHierarchyParam with rebound bindings at
                //    each interface-inheritance step.
                if (!result) {
                    auto currentClass = classDef;
                    while (currentClass && !result) {
                        const auto& interfaces = currentClass->getImplementedInterfaces();
                        for (const auto& iface : interfaces) {
                            if (targetHasParams) {
                                // MYT-44: substitute declaration-time type args
                                // (e.g. "List<E>") using the receiver's runtime
                                // bindings ({"E": "Int"}) to produce "List<Int>",
                                // then compare against the target.
                                std::string substituted = substituteTypeExpression(
                                    iface, obj->getGenericTypeBindings());
                                if (substituted == targetTypeName) {
                                    result = true;
                                    break;
                                }
                                // Recurse through interface-on-interface
                                // inheritance, rebinding at each step so
                                // parameter names can differ across the chain.
                                std::unordered_set<std::string> visited;
                                if (checkInterfaceHierarchyParam(
                                        substituted, targetTypeName, visited,
                                        obj->getGenericTypeBindings(), env)) {
                                    result = true;
                                    break;
                                }
                            } else {
                                // MYT-41 raw path, unchanged.
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

                        // Move to parent class
                        currentClass = currentClass->getParentClass();
                    }
                }

                return result;
            } else {
                return false; // null is not instance of any type
            }
        }
        // ValueObject type check (value classes)
        if (value::isValueObject(val)) {
            auto obj = value::asValueObject(val);
            if (obj) {
                auto classDef = obj->getClassDefinition();
                // Reconstruct the full parameterized type from bindings —
                // value classes CAN be generic, so we mirror the ObjectInstance
                // path. Note that most value-class instances won't have
                // bindings, so this reduces to classDef->getName() for them.
                std::string className = reconstructValueObjectFullType(obj);
                std::string baseClassName = ::types::TypeConversionUtils::extractBaseTypeName(className);
                std::string baseTargetName = ::types::TypeConversionUtils::extractBaseTypeName(targetTypeName);
                bool targetHasParams = (targetTypeName.find('<') != std::string::npos);

                bool result = false;
                if (className == targetTypeName) {
                    result = true;
                } else if (!targetHasParams && baseClassName == baseTargetName) {
                    result = true;
                }

                // Check implemented interfaces — mirrors the ObjectInstance
                // branch above for value classes. Raw target uses the
                // existing checkInterfaceHierarchy path; parameterized
                // target uses MYT-44's substitution-based walk.
                if (!result) {
                    const auto& interfaces = classDef->getImplementedInterfaces();
                    for (const auto& iface : interfaces) {
                        if (targetHasParams) {
                            std::string substituted = substituteTypeExpression(
                                iface, obj->getGenericTypeBindings());
                            if (substituted == targetTypeName) {
                                result = true;
                                break;
                            }
                            std::unordered_set<std::string> visited;
                            if (checkInterfaceHierarchyParam(
                                    substituted, targetTypeName, visited,
                                    obj->getGenericTypeBindings(), env)) {
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
                }

                return result;
            }
            return false;
        }
        else if (utils::isNullValue(val)) {
            return false; // null is not instance of any type
        }
        else {
            return false; // primitive values are not instances of object types
        }
    }

    value::Value TypeExecutor::castToInt(const value::Value& val) {
        if (value::isInt(val)) {
            return val; // Already int
        }
        else if (value::isFloat(val)) {
            return static_cast<int64_t>(value::asFloat(val));
        }
        else if (value::isBool(val)) {
            return value::asBool(val) ? static_cast<int64_t>(1) : static_cast<int64_t>(0);
        }
        else if (value::isAnyString(val)) {
            // MYT-317: folds STRING_INLINE alongside heap STRING / INTERNED_STRING.
            std::string str(value::asStringView(val));
            try {
                return static_cast<int64_t>(std::stoll(str));
            } catch (...) {
                throwCastError("Cannot cast string to int: " + str);
            }
        }
        else if (value::isAnyObject(val)) {
            // MYT-208: accept STACK_OBJECT (cast on a stack-promoted Int box).
            auto* obj = value::asObjectInstanceRaw(val);
            if (obj && obj->getClassDefinition()->getName() == "Int") {
                // Return the Int object itself (it's already the right type)
                return val;
            }
            throwCastError("Cannot cast object to int");
        }
        else if (value::isValueObject(val)) {
            auto obj = value::asValueObject(val);
            if (obj && obj->getClassName() == "Int") {
                return val;
            }
            throwCastError("Cannot cast value object to int");
        }
        else {
            throwCastError("Cannot cast to int from this type");
        }
    }

    value::Value TypeExecutor::castToFloat(const value::Value& val) {
        if (value::isFloat(val)) {
            return val; // Already float
        }
        else if (value::isInt(val)) {
            return static_cast<double>(value::asInt(val));
        }
        else if (value::isAnyString(val)) {
            // MYT-317: SSO-aware string→float cast.
            std::string str(value::asStringView(val));
            try {
                return std::stod(str);
            } catch (...) {
                throwCastError("Cannot cast string to float: " + str);
            }
        }
        else if (value::isAnyObject(val)) {
            auto* obj = value::asObjectInstanceRaw(val);
            if (obj && obj->getClassDefinition()->getName() == "Float") {
                return val;
            }
            throwCastError("Cannot cast object to float");
        }
        else if (value::isValueObject(val)) {
            auto obj = value::asValueObject(val);
            if (obj && obj->getClassName() == "Float") {
                return val;
            }
            throwCastError("Cannot cast value object to float");
        }
        else {
            throwCastError("Cannot cast to float from this type");
        }
    }

    value::Value TypeExecutor::castToString(const value::Value& val) {
        // A String boxed-class instance must pass through unchanged — unboxing
        // to a primitive string here would strip the wrapper and break later
        // method dispatch (e.g. `(String)list.get(i)` followed by `.getValue()`).
        // Mirrors the isAnyObject / isValueObject pass-through in castToInt.
        if (value::isAnyObject(val)) {
            auto* obj = value::asObjectInstanceRaw(val);
            if (obj && obj->getClassDefinition()->getName() == "String") {
                return val;
            }
        }
        if (value::isValueObject(val)) {
            auto obj = value::asValueObject(val);
            if (obj && obj->getClassName() == "String") {
                return val;
            }
        }
        return valueToString(val);
    }

    value::Value TypeExecutor::castToBool(const value::Value& val) {
        if (value::isBool(val)) {
            return val; // Already bool
        }
        else if (value::isInt(val)) {
            return value::asInt(val) != 0;
        }
        else if (value::isFloat(val)) {
            return value::asFloat(val) != 0.0;
        }
        else if (value::isAnyString(val)) {
            // MYT-317: SSO-aware. Non-empty strings of any form are true.
            return !value::asStringView(val).empty();
        }
        else if (value::isAnyObject(val)) {
            auto* obj = value::asObjectInstanceRaw(val);
            if (obj && obj->getClassDefinition()->getName() == "Bool") {
                return val;
            }
            throwCastError("Cannot cast object to bool");
        }
        else if (value::isValueObject(val)) {
            auto obj = value::asValueObject(val);
            if (obj && obj->getClassName() == "Bool") {
                return val;
            }
            throwCastError("Cannot cast value object to bool");
        }
        else {
            throwCastError("Cannot cast to bool from this type");
        }
    }

    TypeExecutor::TypeComponents TypeExecutor::extractTypeComponents(const std::string& typeName) {
        TypeComponents components;
        components.baseName = typeName;
        components.typeParams = "";

        size_t genericStart = typeName.find('<');
        if (genericStart != std::string::npos) {
            components.baseName = typeName.substr(0, genericStart);
            components.typeParams = typeName.substr(genericStart); // includes <>
        }

        return components;
    }

    bool TypeExecutor::checkExactMatch(const std::string& className, const std::string& targetTypeName,
                                       const TypeComponents& classComp, const TypeComponents& targetComp) {
        // 1. Exact match (including generic type parameters)
        if (className == targetTypeName) {
            return true;
        }
        // 2. Base class match without type parameters (e.g., Box<Int> -> Box)
        if (classComp.baseName == targetComp.baseName && targetComp.typeParams.empty()) {
            return true;
        }
        // 3. Type erasure compatibility: allow casting from erased type to parameterized type
        // (e.g., Box -> Box<Int>). This happens because mType uses type erasure at runtime.
        if (classComp.baseName == targetComp.baseName && classComp.typeParams.empty()) {
            return true;
        }
        return false;
    }

    bool TypeExecutor::checkUpcastMatch(std::shared_ptr<runtimeTypes::klass::ClassDefinition> classDef,
                                        const std::string& targetTypeName, const TypeComponents& targetComp,
                                        const std::string& baseClassName, const std::string& classTypeParams) {
        // 3. Upcast - object is subclass of target type
        if (classDef->isSubclassOf(targetTypeName)) {
            return true;
        }
        // 4. Upcast with base names (for generic types)
        if (classDef->isSubclassOf(targetComp.baseName)) {
            // Check if type parameters match (if target has type params)
            if (!targetComp.typeParams.empty() && !classTypeParams.empty()) {
                // Type parameters must match for generic upcast
                return (classTypeParams == targetComp.typeParams);
            }
            // No type params on target, allow upcast to generic base
            return true;
        }
        return false;
    }

    bool TypeExecutor::checkDowncastMatch(const std::string& baseClassName, const std::string& baseTargetName,
                                          const std::string& classTypeParams, const std::string& targetTypeParams) {
        // Get target class from registry to check if it's a subclass
        auto targetClass = environment->findClass(baseTargetName);
        if (!targetClass) {
            return false;
        }

        // Check if target class is a subclass of the object's actual class
        if (!targetClass->isSubclassOf(baseClassName)) {
            return false;
        }

        // Check type parameter compatibility for generic downcast
        if (!targetTypeParams.empty() && !classTypeParams.empty()) {
            // Type parameters must match for valid generic downcast
            return (classTypeParams == targetTypeParams);
        }
        // No type param mismatch, allow downcast
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

    void TypeExecutor::throwIncompatibleCastError(const std::string& className, const std::string& targetTypeName) {
        throwCastError("Cannot cast " + className + " to " + targetTypeName +
                      ": incompatible types in inheritance hierarchy");
    }

    void TypeExecutor::throwCastError(const std::string& message) {
        // Get source location
        auto* loc = context.program->getSourceLocation(context.instructionPointer);
        errors::SourceLocation errorLoc = loc ?
            errors::SourceLocation(loc->filename, loc->line, loc->column) :
            errors::SourceLocation();

        // Throw as UserException with type "Exception" so it can be caught by catch(Exception e)
        // In the future, we could create actual CastError exception objects that extend Exception
        throw errors::UserException(message, "Exception", errorLoc);
    }

    value::Value TypeExecutor::castToObject(const value::Value& val, const std::string& targetTypeName) {
        // Handle null values
        if (utils::isNullValue(val)) {
            return val; // null remains null for object casts
        }

        // Handle ValueObject (value classes)
        if (value::isValueObject(val)) {
            auto obj = value::asValueObject(val);
            if (!obj) {
                throwCastError("Cannot cast null to " + targetTypeName);
            }
            auto classDef = obj->getClassDefinition();
            std::string className = classDef->getName();

            TypeComponents classComp = extractTypeComponents(className);
            TypeComponents targetComp = extractTypeComponents(targetTypeName);

            bool canCast = checkExactMatch(className, targetTypeName, classComp, targetComp) ||
                           checkInterfaceMatch(classDef, targetTypeName);

            if (canCast) {
                return val;
            } else {
                throwIncompatibleCastError(className, targetTypeName);
                return val;
            }
        }

        // Handle non-object types.
        // MYT-208: accept STACK_OBJECT — `(Point) p` on a stack-promoted local
        // is valid and cheap; raw pointer access is enough for the cast.
        if (!value::isAnyObject(val)) {
            throwCastError("Cannot cast primitive type to " + targetTypeName);
        }

        // Object cast - check if it's a valid object type
        auto* obj = value::asObjectInstanceRaw(val);
        if (!obj) {
            throwCastError("Cannot cast null to " + targetTypeName);
        }

        auto classDef = obj->getClassDefinition();
        std::string className = classDef->getName();

        // Extract base class name and type parameters
        TypeComponents classComp = extractTypeComponents(className);
        TypeComponents targetComp = extractTypeComponents(targetTypeName);

        // Check if the object's class can be cast to target type
        bool canCast = checkExactMatch(className, targetTypeName, classComp, targetComp) ||
                       checkUpcastMatch(classDef, targetTypeName, targetComp, classComp.baseName, classComp.typeParams) ||
                       checkDowncastMatch(classComp.baseName, targetComp.baseName, classComp.typeParams, targetComp.typeParams) ||
                       checkInterfaceMatch(classDef, targetTypeName);

        if (canCast) {
            // Cast is a runtime type check — preserve the original Value's tag
            // (OBJECT or STACK_OBJECT). Returning the raw `obj` pointer here
            // would silently match Value(bool) via pointer→bool decay.
            return val;
        } else {
            throwIncompatibleCastError(className, targetTypeName);
            return val; // Never reached, but satisfies return type
        }
    }

    bool TypeExecutor::checkInterfaceHierarchy(
        const std::string& interfaceName,
        const std::string& targetInterface,
        std::unordered_set<std::string>& visited,
        environment::Environment* env
    ) {
        // Avoid infinite loops with circular dependencies
        if (visited.count(interfaceName)) {
            return false;
        }
        visited.insert(interfaceName);

        // Strip generic parameters for comparison
        auto stripGenerics = [](const std::string& name) -> std::string {
            return ::types::TypeConversionUtils::extractBaseTypeName(name);
        };

        std::string interfaceBaseName = stripGenerics(interfaceName);
        std::string targetBaseName = stripGenerics(targetInterface);

        // Extract type parameters
        std::string interfaceTypeParams = "";
        size_t ifaceGenericStart = interfaceName.find('<');
        if (ifaceGenericStart != std::string::npos) {
            interfaceTypeParams = interfaceName.substr(ifaceGenericStart);
        }

        std::string targetTypeParams = "";
        size_t targetGenericStart = targetInterface.find('<');
        if (targetGenericStart != std::string::npos) {
            targetTypeParams = targetInterface.substr(targetGenericStart);
        }

        // Direct match (exact or base without type params)
        if (interfaceName == targetInterface ||
            (interfaceBaseName == targetBaseName && targetTypeParams.empty())) {
            return true;
        }
        // Type param match
        if (interfaceBaseName == targetBaseName && !targetTypeParams.empty()) {
            return (interfaceTypeParams == targetTypeParams);
        }

        // Check extended interfaces recursively
        auto interfaceDef = env ? env->findInterface(interfaceName) : nullptr;
        if (interfaceDef) {
            const auto& extendedInterfaces = interfaceDef->getExtendedInterfaces();
            for (const auto& extendedInterface : extendedInterfaces) {
                if (checkInterfaceHierarchy(extendedInterface, targetInterface, visited, env)) {
                    return true;
                }
            }
        }

        return false;
    }

    // MYT-44: parameterized interface-hierarchy walk.
    //
    // `interfaceName` is already substituted (e.g. "SortedList<Int>") — the
    // caller resolved the receiver's bindings before invoking us. At each
    // recursion step we look up the interface's definition, rebuild the
    // binding map using the interface's OWN declared parameters (which may
    // use different letters than the caller), then substitute its extended
    // interfaces and recurse.
    //
    // Split from the raw-mode overload because the two modes have materially
    // different recursion contracts (bindings must travel alongside the name
    // in parameterized mode) and MYT-44 explicitly does not want to risk
    // regressing the raw path.
    bool TypeExecutor::checkInterfaceHierarchyParam(
        const std::string& interfaceName,
        const std::string& targetInterface,
        std::unordered_set<std::string>& visited,
        const std::unordered_map<std::string, std::string>& bindings,
        environment::Environment* env
    ) {
        // Avoid infinite loops with circular dependencies. Use the full
        // substituted name as the visited key so two different parameterized
        // views of the same template (e.g. SortedList<Int> vs SortedList<String>
        // reached through different paths) don't collide.
        if (visited.count(interfaceName)) {
            return false;
        }
        visited.insert(interfaceName);

        // Direct match on the already-substituted form.
        if (interfaceName == targetInterface) {
            return true;
        }

        // Split current interface into base + params so we can both look up
        // the InterfaceDefinition (by raw base name) AND feed the substituted
        // args into rebindForInterface for the recursion step.
        TypeComponents currentComp = extractTypeComponents(interfaceName);

        // Look up the interface by its raw base name. If it's not registered
        // as an interface, we can't walk further — return the direct match
        // result we already checked.
        auto interfaceDef = env ? env->findInterface(currentComp.baseName) : nullptr;
        if (!interfaceDef) {
            return false;
        }

        // Rebuild the binding map using the interface's own declared params.
        // When arity matches, this gives us new bindings keyed by the
        // interface's parameter names. When it doesn't (e.g. a raw extends
        // clause or malformed input), rebindForInterface returns empty and
        // any substitutions in this recursion step will fall back to the
        // verbatim behavior — which is still correct for raw targets but
        // will fail the parameterized match, as desired.
        auto newBindings = rebindForInterface(interfaceDef.get(), currentComp.typeParams);

        // Walk the extended-interfaces list, substituting each entry against
        // the new bindings and recursing with them.
        const auto& extendedInterfaces = interfaceDef->getExtendedInterfaces();
        for (const auto& extendedInterface : extendedInterfaces) {
            std::string substituted = substituteTypeExpression(extendedInterface, newBindings);
            if (checkInterfaceHierarchyParam(substituted, targetInterface, visited, newBindings, env)) {
                return true;
            }
        }

        return false;
    }

    std::string TypeExecutor::valueToString(const value::Value& val) {
        if (value::isInt(val)) {
            return std::to_string(value::asInt(val));
        }
        if (value::isFloat(val)) {
            std::ostringstream oss;
            oss << value::asFloat(val);
            return oss.str();
        }
        if (value::isBool(val)) {
            return value::asBool(val) ? "true" : "false";
        }
        // MYT-317: SSO-aware stringify.
        if (value::isAnyString(val)) {
            return std::string(value::asStringView(val));
        }
        if (utils::isNullValue(val)) {
            return "null";
        }
        if (value::isValueObject(val)) {
            auto obj = value::asValueObject(val);
            if (obj && obj->hasField("value") && obj->getFieldCount() == 1) {
                return valueToString(obj->getFieldValue("value"));
            }
            return obj ? "<" + obj->getClassName() + ">" : "null";
        }
        // MYT-208: accept STACK_OBJECT alongside OBJECT — `(string) p` on a
        // stack-promoted local should produce the wrapper's "value" field
        // when present, mirroring the OBJECT-tag fallback.
        if (value::isAnyObject(val)) {
            auto* obj = value::asObjectInstanceRaw(val);
            if (obj && obj->getField("value")) {
                return valueToString(obj->getFieldValue("value"));
            }
        }
        return "<object>";
    }

    std::string TypeExecutor::buildParameterizedName(
        const std::shared_ptr<runtimeTypes::klass::ClassDefinition>& classDef,
        const std::unordered_map<std::string, std::string>& bindings) {
        const auto& declaredParams = classDef->getGenericParameters();
        if (declaredParams.empty()) {
            return classDef->getName();
        }

        // Partial bindings → type-erased fallback (raw base name). Matches
        // the existing backward-compat behavior for instances created via
        // `new Box(...)` without explicit type arguments.
        std::vector<std::string> args;
        args.reserve(declaredParams.size());
        for (const auto& p : declaredParams) {
            auto it = bindings.find(p.name);
            if (it == bindings.end() || it->second.empty()) {
                return classDef->getName();
            }
            args.push_back(it->second);
        }

        // Spacing MUST match ast::GenericType::toString() exactly ("<", ", ", ">")
        // so the compiler-emitted target string and the runtime-reconstructed
        // receiver string are comparable via raw string equality.
        std::string out = classDef->getName();
        out += "<";
        for (size_t i = 0; i < args.size(); ++i) {
            if (i > 0) out += ", ";
            out += args[i];
        }
        out += ">";
        return out;
    }

    std::string TypeExecutor::reconstructObjectFullType(
        const runtimeTypes::klass::ObjectInstance* obj) {
        if (!obj) {
            return "";
        }
        return buildParameterizedName(obj->getClassDefinition(),
                                      obj->getGenericTypeBindings());
    }

    std::string TypeExecutor::reconstructValueObjectFullType(
        const std::shared_ptr<value::ValueObject>& obj) {
        if (!obj) {
            return "";
        }
        return buildParameterizedName(obj->getClassDefinition(),
                                      obj->getGenericTypeBindings());
    }

    std::string TypeExecutor::reconstructMultiArrayTypeName(const value::Value& val) {
        const value::Value* defaultPtr = nullptr;
        size_t rank = 0;
        if (value::isFlatMultiArray(val)) {
            const auto& arr = value::asFlatMultiArray(val);
            defaultPtr = &arr->getDefaultValue();
            rank = arr->getRank();
        } else if (value::isSparseMultiArray(val)) {
            const auto& arr = value::asSparseMultiArray(val);
            defaultPtr = &arr->getDefaultValue();
            rank = arr->getRank();
        }
        if (!defaultPtr || rank == 0) {
            return "";
        }
        const char* elemName = nullptr;
        switch (defaultPtr->tag()) {
            case value::ValueType::INT:    elemName = "int";    break;
            case value::ValueType::FLOAT:  elemName = "float";  break;
            case value::ValueType::BOOL:   elemName = "bool";   break;
            case value::ValueType::STRING: elemName = "string"; break;
            default: break;
        }
        if (!elemName) {
            return "";
        }
        std::string fullName(elemName);
        fullName.reserve(fullName.size() + rank * 2);
        for (size_t i = 0; i < rank; ++i) {
            fullName += "[]";
        }
        return fullName;
    }
}
