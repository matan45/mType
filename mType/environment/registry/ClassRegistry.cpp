#include "ClassRegistry.hpp"

namespace environment::registry
{
    ClassRegistry::ClassRegistry()
        : inheritanceTracker(std::make_unique<InheritanceTracker>()),
          reifiedTypeRegistry(std::make_shared<::types::ReifiedTypeRegistry>())
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

    void ClassRegistry::clearScriptDefinitions()
    {
        // Preserve built-in classes (registered by EnvironmentBuilder, not by scripts)
        static const std::vector<std::string> builtinClasses = {"Object"};
        std::unordered_map<std::string, std::shared_ptr<runtimeTypes::klass::ClassDefinition>> preserved;
        for (const auto& name : builtinClasses)
        {
            auto it = items.find(name);
            if (it != items.end())
                preserved.insert(*it);
        }
        items.clear();
        items.insert(preserved.begin(), preserved.end());

        inheritanceTracker->clear();
        reifiedTypeRegistry->clear();
    }

    // NEW (Phase 4): Reified type management
    ::types::UnifiedTypePtr ClassRegistry::getReifiedClassType(
        const std::string& className,
        const std::vector<::types::UnifiedTypePtr>& typeArguments)
    {
        // Create the class type with type arguments
        auto classType = ::types::UnifiedType::classType(className, typeArguments);

        // Intern through the reified type registry to ensure identity
        return reifiedTypeRegistry->intern(classType);
    }

    bool ClassRegistry::isSameReifiedType(
        const ::types::UnifiedTypePtr& type1,
        const ::types::UnifiedTypePtr& type2) const
    {
        if (!type1 || !type2)
        {
            return type1 == type2; // Both null = same
        }

        // Use canonical string comparison for type identity
        return type1->toCanonicalString() == type2->toCanonicalString();
    }
}
