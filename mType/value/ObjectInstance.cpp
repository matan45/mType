#include "ObjectInstance.hpp"
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <unordered_set>
#include "../gc/GC.hpp"
#include "ValueShim.hpp"

namespace runtimeTypes::klass
{

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4413)  // offsetof on non-standard-layout (enable_shared_from_this base)
#endif
    size_t ObjectInstance::classDefinitionMemberOffset() noexcept
    {
        return offsetof(ObjectInstance, classDefinition);
    }
#ifdef _MSC_VER
#pragma warning(pop)
#endif

    void ObjectInstance::registerWithGC()
    {
        if (!gcRegistered && gc::GC::isInitialized())
        {
            gc::GC::registerAllocation(shared_from_this(), gc::config::GCObjectType::OBJECT_INSTANCE);
            gcRegistered = true;
        }
    }

    void ObjectInstance::resetForRecycle()
    {
        // unordered_map::clear() destroys nodes (releasing Value bridges)
        // but keeps the bucket array.
        fieldValues.clear();
        genericTypeBindings.clear();
        fieldVector.clear();
        specializedCollection_.reset();
        gcRegistered = false;
        primitiveTag_ = value::PrimitiveTypeTag::NONE;
        classDefinition.reset();
    }

    void ObjectInstance::reinitForRecycle(
        std::shared_ptr<ClassDefinition> classDef,
        const std::unordered_map<std::string, std::string>& typeBindings)
    {
        classDefinition = std::move(classDef);
        if (!typeBindings.empty())
        {
            genericTypeBindings = typeBindings;
        }
        if (classDefinition)
        {
            primitiveTag_ = value::classNameToPrimitiveTag(classDefinition->getName());
            ensureFieldVector();
        }
    }

    std::shared_ptr<FieldDefinition> ObjectInstance::getField(const std::string& fieldName) const
    {
        if (!classDefinition) return nullptr;
        return classDefinition->getFieldInHierarchy(fieldName);
    }

    Value ObjectInstance::getFieldValue(const std::string& fieldName) const
    {
        auto field = getField(fieldName);
        if (field) {
            if (field->isStatic()) {
                return field->getValue();
            }

            size_t idx = classDefinition->getFieldIndex(fieldName);
            if (idx != SIZE_MAX && idx < fieldVector.size()) {
                return fieldVector[idx];
            }

            return field->getValue();
        } else {
            // Lambda backing fields and other dynamic fields are stored in fieldValues.
            auto it = fieldValues.find(fieldName);
            if (it != fieldValues.end()) {
                return it->second;
            }
        }
        return std::monostate{};
    }

    std::shared_ptr<ClassDefinition> ObjectInstance::getClassDefinition() const
    {
        return classDefinition;
    }

    void ObjectInstance::setField(const std::string& fieldName, const Value& value)
    {
        // MYT-208 defense-in-depth: STACK_OBJECT is a borrowed pointer whose
        // lifetime is bounded by the owning CallFrame's stackObjects array.
        // Persisting it into a field could outlive that frame. The escape
        // analyzer in optimizer/passes/EscapeAnalysisPass.cpp walks
        // MemberAssignmentNode::getValue() in Ctx::ESCAPING precisely to
        // prevent this, so the assert backs that static guarantee.
        assert(!value::isStackObject(value) &&
               "ObjectInstance::setField: STACK_OBJECT escaped its frame — "
               "EscapeAnalysisPass should have demoted this allocation");

        auto field = getField(fieldName);

        if (field) {
            if (field->isStatic()) {
                Value oldValue = field->getValue();
                void* oldPtr = gc::extractPointer(oldValue);
                void* newPtr = gc::extractPointer(value);
                if (oldPtr != nullptr && oldPtr != newPtr)
                {
                    gc::GC::onRefCountDecrement(oldPtr);
                }
                field->setValue(value);
            } else {
                size_t idx = classDefinition->getFieldIndex(fieldName);
                if (idx != SIZE_MAX && idx < fieldVector.size()) {
                    void* newPtr = gc::extractPointer(value);
                    void* oldPtr = gc::extractPointer(fieldVector[idx]);
                    if (oldPtr != nullptr && oldPtr != newPtr)
                    {
                        gc::GC::onRefCountDecrement(oldPtr);
                    }
                    if (newPtr != nullptr && gcRegistered)
                    {
                        gc::GC::onRefCountDecrement(this);
                    }
                    fieldVector[idx] = value;
                } else {
                    fieldValues[fieldName] = value;
                }
            }
        } else {
            // Allow dynamic fields not declared on the class — e.g. lambda backing fields.
            auto it = fieldValues.find(fieldName);
            void* newPtr = gc::extractPointer(value);
            if (it != fieldValues.end())
            {
                void* oldPtr = gc::extractPointer(it->second);
                if (oldPtr != nullptr && oldPtr != newPtr)
                {
                    gc::GC::onRefCountDecrement(oldPtr);
                }
            }
            if (newPtr != nullptr && gcRegistered)
            {
                gc::GC::onRefCountDecrement(this);
            }
            fieldValues[fieldName] = value;
        }
    }

    void ObjectInstance::ensureFieldVector()
    {
        if (!classDefinition) return;

        size_t count = classDefinition->getIndexedFieldCount();
        if (fieldVector.size() != count) {
            fieldVector.resize(count, std::monostate{});
        }
    }

    const Value& ObjectInstance::getFieldByIndex(size_t index) const
    {
        static const Value nullValue = std::monostate{};
        if (index >= fieldVector.size()) return nullValue;
        return fieldVector[index];
    }

    void ObjectInstance::setFieldByIndex(size_t index, const Value& value)
    {
        // MYT-208 defense-in-depth: see setField for rationale.
        assert(!value::isStackObject(value) &&
               "ObjectInstance::setFieldByIndex: STACK_OBJECT escaped its frame — "
               "EscapeAnalysisPass should have demoted this allocation");

        if (index >= fieldVector.size()) return;

        void* newPtr = gc::extractPointer(value);
        void* oldPtr = gc::extractPointer(fieldVector[index]);
        if (oldPtr != nullptr && oldPtr != newPtr)
        {
            gc::GC::onRefCountDecrement(oldPtr);
        }
        if (newPtr != nullptr && gcRegistered)
        {
            gc::GC::onRefCountDecrement(this);
        }

        fieldVector[index] = value;
    }

    std::vector<std::pair<std::string, Value>> ObjectInstance::getAllFields() const
    {
        std::vector<std::pair<std::string, Value>> result;
        std::unordered_set<std::string> seen;
        if (classDefinition) {
            const auto& names = classDefinition->getFieldIndexToName();
            for (size_t i = 0; i < names.size(); ++i) {
                if (i < fieldVector.size()) {
                    seen.insert(names[i]);
                    result.emplace_back(names[i], fieldVector[i]);
                }
            }
        }
        // Declared-name collision can occur if setField wrote a name into the
        // dynamic fallback before the classDef was bound (e.g. via the null-
        // classDef path on a partially-initialised instance). Skip the dup so
        // consumers like JsonSerializer don't see the same key twice.
        for (const auto& pair : fieldValues) {
            if (seen.count(pair.first)) continue;
            result.emplace_back(pair.first, pair.second);
        }
        return result;
    }

    bool ObjectInstance::isInstanceOf(const std::string& className) const
    {
        if (!classDefinition) {
            return false;
        }

        if (classDefinition->getName() == className) {
            return true;
        }

        return classDefinition->isSubclassOf(className);
    }

    std::string ObjectInstance::getTypeName() const
    {
        return classDefinition ? classDefinition->getName() : "unknown";
    }
}
