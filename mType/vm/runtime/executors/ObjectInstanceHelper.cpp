#include "ObjectInstanceHelper.hpp"
#include "../VirtualMachine.hpp"
#include "../../../value/ObjectInstancePool.hpp"
#include "../../../errors/RuntimeException.hpp"
#include "../../../errors/TypeException.hpp"
#include "../../../gc/GC.hpp"
#include <algorithm>

namespace vm::runtime
{
    namespace
    {
        // Parse type arguments with proper bracket depth tracking so nested
        // generics like "Container<HashMap<String, List<Int>>>" split correctly.
        std::vector<std::string> parseTypeArguments(const std::string& typeArgsStr)
        {
            std::vector<std::string> typeArgs;
            std::string current;
            int depth = 0;

            for (char c : typeArgsStr)
            {
                if (c == '<')
                {
                    depth++;
                    current += c;
                }
                else if (c == '>')
                {
                    depth--;
                    current += c;
                }
                else if (c == ',' && depth == 0)
                {
                    if (!current.empty())
                    {
                        size_t start = current.find_first_not_of(" \t");
                        size_t end = current.find_last_not_of(" \t");
                        if (start != std::string::npos && end != std::string::npos)
                        {
                            typeArgs.push_back(current.substr(start, end - start + 1));
                        }
                    }
                    current.clear();
                }
                else
                {
                    current += c;
                }
            }

            if (!current.empty())
            {
                size_t start = current.find_first_not_of(" \t");
                size_t end = current.find_last_not_of(" \t");
                if (start != std::string::npos && end != std::string::npos)
                {
                    typeArgs.push_back(current.substr(start, end - start + 1));
                }
            }

            return typeArgs;
        }
    }

    ObjectInstanceHelper::ObjectInstanceHelper(ExecutionContext& ctx,
                                               std::shared_ptr<environment::Environment> env,
                                               VirtualMachine* vmPtr)
        : context(ctx)
        , environment(std::move(env))
        , vm(vmPtr)
    {
    }

    std::string ObjectInstanceHelper::parseGenericTypeArguments(const std::string& fullClassName,
                                                               std::unordered_map<std::string, std::string>& genericTypeBindings)
    {
        std::string baseClassName = fullClassName;
        size_t genericStart = fullClassName.find('<');

        if (genericStart == std::string::npos) {
            return baseClassName;
        }

        baseClassName = fullClassName.substr(0, genericStart);
        size_t genericEnd = fullClassName.rfind('>');
        if (genericEnd == std::string::npos || genericEnd <= genericStart) {
            return baseClassName;
        }

        std::string typeArgsStr = fullClassName.substr(genericStart + 1, genericEnd - genericStart - 1);

        std::vector<std::string> typeArgs = parseTypeArguments(typeArgsStr);

        for (const auto& typeArg : typeArgs) {
            if (typeArg.empty()) {
                throw errors::TypeException("Invalid empty type argument for generic class '" + baseClassName + "'");
            }

            if (typeArg == "void" && baseClassName == "Promise") {
                continue;
            }

            // PURE OOP: Primitives are allowed as generic type arguments and
            // are treated as their Box class equivalents (Int, Float, Bool, String).
        }

        if (!environment) {
            throw errors::RuntimeException("Environment not available when parsing generic type arguments for class: " + baseClassName);
        }

        auto classRegistry = environment->getClassRegistry();
        if (!classRegistry) {
            throw errors::RuntimeException("Class registry not available when parsing generic type arguments for class: " + baseClassName);
        }

        auto classDef = classRegistry->findClass(baseClassName);
        if (!classDef) {
            throw errors::RuntimeException("Class definition not found when parsing generic type arguments for class: " + baseClassName);
        }

        const auto& genericParams = classDef->getGenericParameters();

        for (size_t i = 0; i < genericParams.size() && i < typeArgs.size(); ++i) {
            genericTypeBindings[genericParams[i].name] = typeArgs[i];
        }

        return baseClassName;
    }

    std::shared_ptr<runtimeTypes::klass::ObjectInstance> ObjectInstanceHelper::createObjectInstance(
        const std::string& baseClassName,
        const std::unordered_map<std::string, std::string>& genericTypeBindings)
    {
        auto classRegistry = environment->getClassRegistry();
        if (!classRegistry) {
            throw errors::RuntimeException("Class registry not available");
        }

        auto classDef = classRegistry->findClass(baseClassName);
        if (!classDef) {
            throw errors::RuntimeException("Class not found: " + baseClassName);
        }

        auto instance = value::ObjectInstancePool::getInstance().acquire(classDef, genericTypeBindings);
        // MYT-208: helper takes raw pointer — pass .get() (lifetime owned by the
        // shared_ptr returned by acquire).
        initializeObjectFields(instance.get(), classDef);

        // GC: Register the newly created object with the garbage collector.
        instance->registerWithGC();

        return instance;
    }

    void ObjectInstanceHelper::initializeObjectFields(
        runtimeTypes::klass::ObjectInstance* instance,
        std::shared_ptr<runtimeTypes::klass::ClassDefinition> classDef)
    {
        // Phase 3b: if every own non-final field is in skipDefaultInitFields
        // (and the parent chain is similarly trivial), the entire init pass
        // is a no-op. Skip the hierarchy walk, vector allocation, reverse,
        // and per-field unordered_map iteration. For tiny short-lived classes
        // like Point (constructor assigns every field, no inheritance) this
        // is the dominant alloc-path cost.
        if (classDef && classDef->isInitTriviallySkippable()) return;

        std::vector<std::shared_ptr<runtimeTypes::klass::ClassDefinition>> hierarchy;
        auto current = classDef;
        int depth = 0;
        while (current) {
            if (depth++ > 100) {
                throw errors::RuntimeException("Circular inheritance detected in class hierarchy");
            }
            hierarchy.push_back(current);
            current = current->getParentClass();
        }
        // Reverse once at end: O(n) total instead of O(n²).
        std::reverse(hierarchy.begin(), hierarchy.end());

        // Phase 2 (allocation perf): only the top-of-hierarchy class (the one
        // being allocated) is safe to skip-default-init for. Parent fields
        // stay fully default-initialised because a child constructor may
        // read `this.<parent_field>` before calling super(), and we don't
        // know the ctor ordering for inherited state at this point.
        for (size_t i = 0; i < hierarchy.size(); ++i) {
            const auto& classInHierarchy = hierarchy[i];
            const bool isSelf = (i == hierarchy.size() - 1);
            for (const auto& [fieldName, fieldDef] : classInHierarchy->getInstanceFields()) {
                // Final fields are always written by the constructor: inline
                // initializers fire in the prologue, ctor-initialized fields
                // fire in the body. Pre-populating them here would make the
                // instance-final check in ObjectExecutor::SET_FIELD see the
                // field as already set and reject the legitimate first write.
                // FieldInitializationValidator catches uninitialized finals
                // at compile time.
                if (fieldDef->isFinal()) continue;

                // Phase 2: the compiler proved every constructor of this
                // class assigns this field before any read. Skipping avoids
                // the unordered_map<string,Value>::insert + write barrier on
                // the allocation hot path.
                if (isSelf && classInHierarchy->shouldSkipDefaultInit(fieldName)) continue;

                value::Value initialValue = fieldDef->getValue();
                if (value::isVoid(initialValue)) {
                    switch (fieldDef->getType()) {
                        case value::ValueType::INT:
                            initialValue = static_cast<int64_t>(0);
                            break;
                        case value::ValueType::FLOAT:
                            initialValue = 0.0;
                            break;
                        case value::ValueType::BOOL:
                            initialValue = false;
                            break;
                        case value::ValueType::STRING:
                            initialValue = std::string("");
                            break;
                        default:
                            initialValue = nullptr;
                            break;
                    }
                }
                // MYT-212: route default-init through THIS class's own slot
                // (not the most-derived slot) so a child shadowing the field
                // doesn't overwrite the parent's value. setField()'s name path
                // would resolve via getFieldInHierarchy → child's slot.
                size_t ownSlot = classInHierarchy->getOwnFieldIndex(fieldName);
                if (ownSlot != SIZE_MAX) {
                    instance->setFieldByIndex(ownSlot, initialValue);
                } else {
                    instance->setField(fieldName, initialValue);
                }
            }
        }
    }

    std::string ObjectInstanceHelper::getCurrentClassName() {
        if (!context.callStack.empty()) {
            // MYT-208: accept stack-promoted `this`.
            if (auto* rawThis = context.callStack.back().getThisInstanceRaw()) {
                return rawThis->getClassDefinition()->getName();
            }
            // MYT-197: prefer frame.definingClassName (populated at push time)
            // over resolving the interned handle and re-splitting.
            if (!context.callStack.back().definingClassName.empty()) {
                return context.callStack.back().definingClassName;
            }
            const std::string& funcName = vm->frameName(context.callStack.back());
            size_t colonPos = funcName.find("::");
            if (colonPos != std::string::npos) {
                return funcName.substr(0, colonPos);
            }
        }
        return "";
    }

    bool ObjectInstanceHelper::isSubclass(const std::string& derivedClass, const std::string& baseClass) {
        if (derivedClass.empty()) return false;
        auto currentClass = environment->getClassRegistry()->findClass(derivedClass);
        while (currentClass && currentClass->hasParentClass()) {
            std::string parentClassName = currentClass->getParentClassName();

            std::string baseParentClassName = parentClassName;
            size_t genericStart = parentClassName.find('<');
            if (genericStart != std::string::npos) {
                baseParentClassName = parentClassName.substr(0, genericStart);
            }

            if (baseParentClassName == baseClass) {
                return true;
            }
            auto parentClass = environment->getClassRegistry()->findClass(baseParentClassName);
            currentClass = parentClass;
        }
        return false;
    }

    validation::AccessContext ObjectInstanceHelper::createAccessContext(
        const std::string& targetClassName,
        bool isSetter)
    {
        std::string currentClassName = getCurrentClassName();
        bool isSameClass = (currentClassName == targetClassName);
        bool isSubclassCheck = isSubclass(currentClassName, targetClassName);

        // Allow SET during static field initialization in global scope.
        if (currentClassName.empty() && context.callStack.empty() && isSetter) {
            isSameClass = true;
        }

        return validation::AccessContext(
            currentClassName,
            targetClassName,
            isSameClass,
            isSubclassCheck,
            isSetter,
            errors::SourceLocation()
        );
    }
}
