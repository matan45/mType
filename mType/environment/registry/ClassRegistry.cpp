#include "ClassRegistry.hpp"
#include <algorithm>
#include <unordered_set>

namespace environment::registry
{
    void ClassRegistry::registerItem(const std::string& name, std::shared_ptr<ClassDefinition> item)
    {
        registerClass(name, item);
    }

    std::shared_ptr<ClassDefinition> ClassRegistry::findItem(const std::string& name) const
    {
        return findClass(name);
    }

    bool ClassRegistry::hasItem(const std::string& name) const
    {
        return hasClass(name);
    }

    void ClassRegistry::removeItem(const std::string& name)
    {
        classes.erase(name);
    }

    std::vector<std::string> ClassRegistry::getAllItemNames() const
    {
        std::vector<std::string> names;
        
        for (const auto& [name, _] : classes)
        {
            names.push_back(name);
        }
        
        
        std::sort(names.begin(), names.end());
        return names;
    }

    size_t ClassRegistry::getItemCount() const
    {
        return classes.size();
    }

    std::string ClassRegistry::getComponentName() const
    {
        return "ClassRegistry";
    }

    void ClassRegistry::registerClass(const std::string& name, std::shared_ptr<ClassDefinition> classDefinition)
    {
        classes[name] = classDefinition;
    }

    std::shared_ptr<ClassDefinition> ClassRegistry::findClass(const std::string& name) const
    {
        auto it = classes.find(name);
        return (it != classes.end()) ? it->second : nullptr;
    }
    

    bool ClassRegistry::hasClass(const std::string& name) const
    {
        return classes.find(name) != classes.end();
    }

    void ClassRegistry::registerInheritance(const std::string& childName, const std::string& parentName)
    {
        // Update parent->children mapping
        parentToChildren[parentName].push_back(childName);

        // Update child->parent mapping
        childToParent[childName] = parentName;

        // Update ClassDefinition relationships if both classes exist
        auto childClass = findClass(childName);
        auto parentClass = findClass(parentName);

        if (childClass && parentClass) {
            childClass->setParentClass(parentClass);
        }
    }

    bool ClassRegistry::wouldCreateCycle(const std::string& childName, const std::string& parentName) const
    {
        std::unordered_set<std::string> visited;
        return wouldCreateCycleHelper(parentName, childName, visited, 0);
    }

    bool ClassRegistry::wouldCreateCycleHelper(const std::string& current, const std::string& target,
                                              std::unordered_set<std::string>& visited, int depth) const
    {
        // Depth protection
        if (depth > MAX_INHERITANCE_DEPTH) {
            return true; // Too deep, assume cycle
        }

        // If we've reached the target, there's a cycle
        if (current == target) {
            return true;
        }

        // Avoid infinite recursion
        if (visited.find(current) != visited.end()) {
            return false;
        }

        visited.insert(current);

        // Check if current has a parent
        auto it = childToParent.find(current);
        if (it != childToParent.end()) {
            return wouldCreateCycleHelper(it->second, target, visited, depth + 1);
        }

        return false;
    }

    std::vector<std::shared_ptr<ClassDefinition>> ClassRegistry::getInheritanceChain(const std::string& className) const
    {
        std::vector<std::shared_ptr<ClassDefinition>> chain;

        auto classIt = childToParent.find(className);
        int depth = 0;

        while (classIt != childToParent.end() && depth < MAX_INHERITANCE_DEPTH) {
            auto parentClass = findClass(classIt->second);
            if (!parentClass) {
                break;
            }

            chain.push_back(parentClass);
            classIt = childToParent.find(classIt->second);
            depth++;
        }

        return chain;
    }

    std::vector<std::string> ClassRegistry::getChildClasses(const std::string& parentName) const
    {
        auto it = parentToChildren.find(parentName);
        if (it != parentToChildren.end()) {
            return it->second;
        }
        return {};
    }

    std::string ClassRegistry::getParentClass(const std::string& childName) const
    {
        auto it = childToParent.find(childName);
        if (it != childToParent.end()) {
            return it->second;
        }
        return "";
    }
}
