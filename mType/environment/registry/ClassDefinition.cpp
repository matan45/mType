#include "ClassDefinition.hpp"
#include <cstddef>
#include <cstdint>
#include "../../value/ValueShim.hpp"

namespace runtimeTypes::klass
{
    bool ClassDefinition::hasField(const std::string& fieldName) const
    {
        return instanceFields.find(fieldName) != instanceFields.end();
    }

    bool ClassDefinition::isInitTriviallySkippable() const
    {
        if (initTriviallySkippableCached) return initTriviallySkippable;

        // A class is trivially-skippable iff:
        //   1. It has no parent class. The existing alloc-path comment
        //      (ObjectInstanceHelper.cpp:313-317) notes that parent fields
        //      stay fully default-init even when the compiler proved the
        //      parent's constructor assigns them, because a child constructor
        //      may read this.<parent_field> before super(). We honor that
        //      conservative invariant — no parent means no such hazard.
        //   2. Every own non-final instance field is in skipDefaultInitFields
        //      (the compiler proved each constructor assigns it before any
        //      read). Final fields are handled by the existing isFinal
        //      early-continue in initializeObjectFields.
        bool result = true;
        if (getParentClass())
        {
            result = false;
        }
        else
        {
            for (const auto& [name, fieldDef] : instanceFields)
            {
                if (fieldDef->isFinal()) continue;
                if (skipDefaultInitFields.count(name) == 0)
                {
                    result = false;
                    break;
                }
            }
        }

        initTriviallySkippable = result;
        initTriviallySkippableCached = true;
        return result;
    }

    bool ClassDefinition::hasMethod(const std::string& methodName) const
    {
        return instanceMethods.find(methodName) != instanceMethods.end();
    }

    bool ClassDefinition::hasStaticField(const std::string& fieldName) const
    {
        return staticFields.find(fieldName) != staticFields.end();
    }

    bool ClassDefinition::hasStaticMethod(const std::string& methodName) const
    {
        return staticMethods.find(methodName) != staticMethods.end();
    }

    ConstructorDefinition* ClassDefinition::findConstructor(size_t argCount) const
    {
        for (const auto& constructor : constructors) {
            if (constructor->getParameterCount() == argCount) {
                return constructor.get();
            }
        }
        return nullptr;
    }

    ConstructorDefinition* ClassDefinition::findConstructorByTypes(std::span<const value::Value> args) const
    {
        size_t argCount = args.size();

        for (const auto& constructor : constructors) {
            if (constructor->getParameterCount() != argCount) {
                continue;
            }

            const auto& params = constructor->getParametersWithTypes();
            bool allMatch = true;

            for (size_t i = 0; i < argCount; ++i) {
                const auto& arg = args[i];
                const auto& paramType = params[i].second;

                value::ValueType argType;
                if (value::isInt(arg)) {
                    argType = value::ValueType::INT;
                } else if (value::isFloat(arg)) {
                    argType = value::ValueType::FLOAT;
                } else if (value::isBool(arg)) {
                    argType = value::ValueType::BOOL;
                } else if (value::isAnyString(arg)) {
                    // MYT-317: STRING_INLINE classifies as STRING for overload resolution.
                    argType = value::ValueType::STRING;
                } else if (value::isVoid(arg) || value::isNullType(arg)) {
                    argType = value::ValueType::VOID;
                } else {
                    argType = value::ValueType::OBJECT;
                }

                if (argType != paramType.basicType) {
                    // Auto-box primitives to their Object wrapper class.
                    if (paramType.basicType == value::ValueType::OBJECT && paramType.className.has_value()) {
                        const std::string& expectedClass = paramType.className.value();
                        if ((expectedClass == "Int" && argType == value::ValueType::INT) ||
                            (expectedClass == "Float" && argType == value::ValueType::FLOAT) ||
                            (expectedClass == "Bool" && argType == value::ValueType::BOOL) ||
                            (expectedClass == "String" && argType == value::ValueType::STRING)) {
                            continue;
                        }
                    }
                    allMatch = false;
                    break;
                }
            }

            if (allMatch) {
                return constructor.get();
            }
        }

        return findConstructor(argCount);
    }

    const std::string& ClassDefinition::getClassName() const
    {
        return getName();
    }

    const std::unordered_map<std::string, std::shared_ptr<FieldDefinition>>& ClassDefinition::getInstanceFields() const
    {
        return instanceFields;
    }

    const std::unordered_map<std::string, std::vector<std::shared_ptr<MethodDefinition>>>& ClassDefinition::getInstanceMethods() const
    {
        return instanceMethods;
    }

    const std::unordered_map<std::string, std::shared_ptr<FieldDefinition>>& ClassDefinition::getStaticFields() const
    {
        return staticFields;
    }

    const std::unordered_map<std::string, std::vector<std::shared_ptr<MethodDefinition>>>& ClassDefinition::getStaticMethods() const
    {
        return staticMethods;
    }

    const std::vector<std::string>& ClassDefinition::getInstanceFieldOrder() const
    {
        return instanceFieldOrder;
    }

    const std::vector<std::string>& ClassDefinition::getStaticFieldOrder() const
    {
        return staticFieldOrder;
    }

    const std::vector<std::string>& ClassDefinition::getInstanceMethodOrder() const
    {
        return instanceMethodOrder;
    }

    const std::vector<std::string>& ClassDefinition::getStaticMethodOrder() const
    {
        return staticMethodOrder;
    }

    const std::vector<std::shared_ptr<ConstructorDefinition>>& ClassDefinition::getConstructors() const
    {
        return constructors;
    }

    void ClassDefinition::addInstanceField(const std::string& name, std::shared_ptr<FieldDefinition> field)
    {
        if (instanceFields.find(name) == instanceFields.end())
        {
            instanceFieldOrder.push_back(name);
        }
        instanceFields[name] = field;
        // Phase 3b: invalidate trivially-skippable cache on field set change.
        initTriviallySkippableCached = false;
    }

    void ClassDefinition::addInstanceMethod(const std::string& name, std::shared_ptr<MethodDefinition> method)
    {
        // MYT-274: compiler-synthesized methods are routed into a separate map
        // so they don't perturb the user-method unordered_map's bucket layout
        // (which would change reflection iteration order).
        if (method && method->isSynthetic())
        {
            syntheticInstanceMethods[name].push_back(method);
            return;
        }
        auto [it, inserted] = instanceMethods.try_emplace(name);
        if (inserted)
        {
            instanceMethodOrder.push_back(name);
        }
        it->second.push_back(method);
    }

    void ClassDefinition::addStaticField(const std::string& name, std::shared_ptr<FieldDefinition> field)
    {
        if (staticFields.find(name) == staticFields.end())
        {
            staticFieldOrder.push_back(name);
        }
        staticFields[name] = field;
    }

    void ClassDefinition::addStaticMethod(const std::string& name, std::shared_ptr<MethodDefinition> method)
    {
        auto [it, inserted] = staticMethods.try_emplace(name);
        if (inserted)
        {
            staticMethodOrder.push_back(name);
        }
        it->second.push_back(method);
    }

    void ClassDefinition::addConstructor(std::shared_ptr<ConstructorDefinition> constructor)
    {
        constructors.push_back(constructor);
    }

    size_t ClassDefinition::getInstanceFieldCount() const
    {
        return instanceFields.size();
    }

    size_t ClassDefinition::getTotalFieldCount() const
    {
        if (totalFieldCountCached) {
            return cachedTotalFieldCount;
        }

        size_t count = instanceFields.size();

        auto parent = parentClass.lock();
        if (parent) {
            count += parent->getTotalFieldCount();
        }

        cachedTotalFieldCount = count;
        totalFieldCountCached = true;
        return count;
    }

    size_t ClassDefinition::getInstanceMethodCount() const
    {
        return instanceMethods.size();
    }

    size_t ClassDefinition::getStaticFieldCount() const
    {
        return staticFields.size();
    }

    size_t ClassDefinition::getStaticMethodCount() const
    {
        return staticMethods.size();
    }

    size_t ClassDefinition::getConstructorCount() const
    {
        return constructors.size();
    }

    void ClassDefinition::addField(std::shared_ptr<FieldDefinition> field)
    {
        if (field->isStatic()) {
            addStaticField(field->getName(), field);
        } else {
            addInstanceField(field->getName(), field);
        }
    }

    void ClassDefinition::addMethod(std::shared_ptr<MethodDefinition> method)
    {
        if (method->isStatic()) {
            addStaticMethod(method->getName(), method);
        } else {
            addInstanceMethod(method->getName(), method);
        }
    }
}
