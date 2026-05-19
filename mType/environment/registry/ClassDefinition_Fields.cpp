#include "ClassDefinition.hpp"
#include <cstddef>
#include <cstdint>

namespace runtimeTypes::klass
{
    void ClassDefinition::buildFieldIndexMap() const
    {
        if (fieldIndexMapBuilt) return;

        fieldIndexMap.clear();
        fieldIndexToName.clear();
        ownFieldIndexMap.clear();

        // Inherit parent class slots so ancestor fields keep their original
        // indices. When this class shadows an ancestor field, the ancestor's
        // value is still reachable via the ancestor's getOwnFieldIndex.
        auto parent = parentClass.lock();
        if (parent)
        {
            parent->buildFieldIndexMap();
            fieldIndexMap = parent->fieldIndexMap;
            fieldIndexToName = parent->fieldIndexToName;
        }

        // MYT-212: append a fresh slot for every field declared in THIS class,
        // even when the name shadows an ancestor. The shadowed inherited entry
        // remains in fieldIndexToName at its old position; fieldIndexMap[name]
        // is overwritten so getFieldIndex(name) — used by the dynamic-dispatch
        // GET_FIELD path — returns the most-derived slot.
        for (const auto& [name, field] : instanceFields)
        {
            size_t index = fieldIndexToName.size();
            fieldIndexToName.push_back(name);
            fieldIndexMap[name] = index;
            ownFieldIndexMap[name] = index;
        }

        fieldIndexMapBuilt = true;
    }

    size_t ClassDefinition::getFieldIndex(const std::string& fieldName) const
    {
        buildFieldIndexMap();
        auto it = fieldIndexMap.find(fieldName);
        if (it != fieldIndexMap.end())
        {
            return it->second;
        }
        return SIZE_MAX;
    }

    size_t ClassDefinition::getOwnFieldIndex(const std::string& fieldName) const
    {
        buildFieldIndexMap();
        auto it = ownFieldIndexMap.find(fieldName);
        if (it != ownFieldIndexMap.end())
        {
            return it->second;
        }
        return SIZE_MAX;
    }

    size_t ClassDefinition::getIndexedFieldCount() const
    {
        buildFieldIndexMap();
        return fieldIndexToName.size();
    }

    const std::unordered_map<std::string, size_t>& ClassDefinition::getFieldIndexMap() const
    {
        buildFieldIndexMap();
        return fieldIndexMap;
    }

    const std::vector<std::string>& ClassDefinition::getFieldIndexToName() const
    {
        buildFieldIndexMap();
        return fieldIndexToName;
    }

    std::shared_ptr<FieldDefinition> ClassDefinition::getField(const std::string& fieldName) const
    {
        auto it = instanceFields.find(fieldName);
        if (it != instanceFields.end()) {
            return it->second;
        }

        auto staticIt = staticFields.find(fieldName);
        if (staticIt != staticFields.end()) {
            return staticIt->second;
        }

        return nullptr;
    }

    std::shared_ptr<FieldDefinition> ClassDefinition::getFieldInHierarchy(const std::string& fieldName) const
    {
        auto field = getField(fieldName);
        if (field) {
            return field;
        }

        auto current = parentClass.lock();
        int depth = 0;

        while (current && depth < MAX_INHERITANCE_DEPTH) {
            field = current->getField(fieldName);
            if (field) {
                return field;
            }
            current = current->parentClass.lock();
            depth++;
        }

        return nullptr;
    }

    std::shared_ptr<ClassDefinition> ClassDefinition::getFieldOwnerInHierarchy(
        const std::string& fieldName,
        std::shared_ptr<ClassDefinition> self) const
    {
        auto field = getField(fieldName);
        if (field) {
            return self;
        }

        auto current = parentClass.lock();
        int depth = 0;

        while (current && depth < MAX_INHERITANCE_DEPTH) {
            field = current->getField(fieldName);
            if (field) {
                return current;
            }
            current = current->parentClass.lock();
            depth++;
        }

        return nullptr;
    }
}
