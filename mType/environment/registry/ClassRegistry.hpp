#pragma once
#include "IRegistry.hpp"
#include "../../runtimeTypes/klass/ClassDefinition.hpp"
#include <unordered_map>
#include <memory>
#include <string>
#include <vector>

namespace environment::registry
{
    using namespace runtimeTypes::klass;

    class ClassRegistry : public IRegistry<ClassDefinition>
    {
    private:
        std::unordered_map<std::string, std::shared_ptr<ClassDefinition>> classes;
        std::unordered_map<std::string, std::unordered_map<std::string, std::shared_ptr<ClassDefinition>>> namespacedClasses;

    public:
        explicit ClassRegistry() = default;
        ~ClassRegistry() override = default;

        void registerItem(const std::string& name, std::shared_ptr<ClassDefinition> item) override;
        std::shared_ptr<ClassDefinition> findItem(const std::string& name) const override;
        std::shared_ptr<ClassDefinition> findQualifiedItem(const std::vector<std::string>& qualifiedName) const override;
        bool hasItem(const std::string& name) const override;
        void removeItem(const std::string& name) override;
        std::vector<std::string> getAllItemNames() const override;
        size_t getItemCount() const override;
        
        std::string getComponentName() const override;
        
        void registerClass(const std::string& name, std::shared_ptr<ClassDefinition> classDefinition);
        void registerClassInNamespace(const std::vector<std::string>& namespacePath, const std::string& className, std::shared_ptr<ClassDefinition> classDefinition);
        std::shared_ptr<ClassDefinition> findClass(const std::string& name) const;
        std::shared_ptr<ClassDefinition> findClassInNamespace(const std::vector<std::string>& namespacePath, const std::string& className) const;
        bool hasClass(const std::string& name) const;
        std::vector<std::string> getClassesInNamespace(const std::vector<std::string>& namespacePath) const;
    };
}
