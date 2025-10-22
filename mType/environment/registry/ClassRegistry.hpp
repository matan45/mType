#pragma once
#include "Registry.hpp"
#include "InheritanceTracker.hpp"
#include "../../runtimeTypes/klass/ClassDefinition.hpp"
#include <memory>
#include <string>
#include <vector>

namespace environment::registry
{
    using namespace runtimeTypes::klass;

    /**
     * @brief Registry for class definitions with inheritance tracking
     *
     * Manages class definitions and delegates inheritance relationship
     * management to InheritanceTracker for better separation of concerns.
     */
    class ClassRegistry : public Registry<ClassDefinition>
    {
    private:
        std::unique_ptr<InheritanceTracker> inheritanceTracker;

    public:
        ClassRegistry();
        ~ClassRegistry() = default;

        // Override removeItem to handle cache invalidation
        void removeItem(const std::string& name) override;

        std::string getComponentName() const override;

        void registerClass(const std::string& name, std::shared_ptr<ClassDefinition> classDefinition);
        std::shared_ptr<ClassDefinition> findClass(const std::string& name) const;
        bool hasClass(const std::string& name) const;

        // Inheritance relationship management (delegated to InheritanceTracker)
        void registerInheritance(const std::string& childName, const std::string& parentName);
        [[nodiscard]] bool wouldCreateCycle(const std::string& childName, const std::string& parentName) const;
        [[nodiscard]] std::vector<std::shared_ptr<ClassDefinition>> getInheritanceChain(const std::string& className) const;
        [[nodiscard]] std::vector<std::string> getChildClasses(const std::string& parentName) const;
        [[nodiscard]] std::string getParentClass(const std::string& childName) const;
    };
}
