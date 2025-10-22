#include "ClassRegistry.hpp"

namespace environment::registry
{
    ClassRegistry::ClassRegistry()
        : inheritanceTracker(std::make_unique<InheritanceTracker>())
    {
        // Set callback so InheritanceTracker can find class definitions
        inheritanceTracker->setFindClassCallback([this](const std::string& name) {
            return findClass(name);
        });
    }

    void ClassRegistry::removeItem(const std::string& name)
    {
        // Call base class to remove from items map
        Registry<ClassDefinition>::removeItem(name);

        // Invalidate inheritance cache
        inheritanceTracker->invalidateCache(name);
    }

    std::string ClassRegistry::getComponentName() const
    {
        return "ClassRegistry";
    }

    void ClassRegistry::registerClass(const std::string& name, std::shared_ptr<ClassDefinition> classDefinition)
    {
        registerItem(name, classDefinition);
    }

    std::shared_ptr<ClassDefinition> ClassRegistry::findClass(const std::string& name) const
    {
        return findItem(name);
    }

    bool ClassRegistry::hasClass(const std::string& name) const
    {
        return hasItem(name);
    }

    void ClassRegistry::registerInheritance(const std::string& childName, const std::string& parentName)
    {
        // Delegate to inheritance tracker
        inheritanceTracker->registerInheritance(childName, parentName);

        // Update ClassDefinition relationships if both classes exist
        auto childClass = findClass(childName);
        auto parentClass = findClass(parentName);

        if (childClass && parentClass)
        {
            childClass->setParentClass(parentClass);
        }
    }

    bool ClassRegistry::wouldCreateCycle(const std::string& childName, const std::string& parentName) const
    {
        return inheritanceTracker->wouldCreateCycle(childName, parentName);
    }

    std::vector<std::shared_ptr<ClassDefinition>> ClassRegistry::getInheritanceChain(const std::string& className) const
    {
        return inheritanceTracker->getInheritanceChain(className);
    }

    std::vector<std::string> ClassRegistry::getChildClasses(const std::string& parentName) const
    {
        return inheritanceTracker->getChildClasses(parentName);
    }

    std::string ClassRegistry::getParentClass(const std::string& childName) const
    {
        return inheritanceTracker->getParentClass(childName);
    }
}
