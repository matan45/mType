#pragma once
#include "Registry.hpp"
#include "InheritanceTracker.hpp"
#include "ClassDefinition.hpp"
#include "../../types/ReifiedTypeRegistry.hpp"
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
     * Also integrates with ReifiedTypeRegistry for generic type reification.
     */
    class ClassRegistry : public Registry<ClassDefinition>
    {
    private:
        std::unique_ptr<InheritanceTracker> inheritanceTracker;
        std::shared_ptr<::types::ReifiedTypeRegistry> reifiedTypeRegistry;

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
        // Virtual so TypeCatalog can invalidate its subtype cache when edges change.
        virtual void registerInheritance(const std::string& childName, const std::string& parentName);
        [[nodiscard]] bool wouldCreateCycle(const std::string& childName, const std::string& parentName) const;
        [[nodiscard]] std::vector<std::shared_ptr<ClassDefinition>> getInheritanceChain(const std::string& className) const;
        [[nodiscard]] std::vector<std::string> getChildClasses(const std::string& parentName) const;
        [[nodiscard]] std::string getParentClass(const std::string& childName) const;

        // Clear all script-defined classes, preserving builtin "Object" class.
        // Virtual so TypeCatalog can also re-bootstrap its primitive/Box state.
        virtual void clearScriptDefinitions();

        // NEW (Phase 4): Reified type management for generic classes
        [[nodiscard]] std::shared_ptr<::types::ReifiedTypeRegistry> getReifiedTypeRegistry() const
        {
            return reifiedTypeRegistry;
        }

        // Get or create a reified type for a generic class instantiation (e.g., Container<String>)
        [[nodiscard]] ::types::UnifiedTypePtr getReifiedClassType(
            const std::string& className,
            const std::vector<::types::UnifiedTypePtr>& typeArguments);

        // Check if two generic class instances are the same reified type
        [[nodiscard]] bool isSameReifiedType(
            const ::types::UnifiedTypePtr& type1,
            const ::types::UnifiedTypePtr& type2) const;
    };
}
