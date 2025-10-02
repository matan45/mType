#pragma once
#include "../../runtimeTypes/klass/ClassDefinition.hpp"
#include <unordered_map>
#include <memory>
#include <string>
#include <vector>

namespace environment::registry
{
    using namespace runtimeTypes::klass;

    class ClassRegistry
    {
    private:
        std::unordered_map<std::string, std::shared_ptr<ClassDefinition>> classes;

        // NEW: Inheritance relationship tracking
        std::unordered_map<std::string, std::vector<std::string>> parentToChildren; // Parent -> List of children
        std::unordered_map<std::string, std::string> childToParent; // Child -> Parent

    public:
        explicit ClassRegistry() = default;
        ~ClassRegistry() = default;

        void registerItem(const std::string& name, std::shared_ptr<ClassDefinition> item);
        std::shared_ptr<ClassDefinition> findItem(const std::string& name) const;
        bool hasItem(const std::string& name) const;
        void removeItem(const std::string& name);
        std::vector<std::string> getAllItemNames() const;
        size_t getItemCount() const;
        
        std::string getComponentName() const;
        
        void registerClass(const std::string& name, std::shared_ptr<ClassDefinition> classDefinition);
        std::shared_ptr<ClassDefinition> findClass(const std::string& name) const;
        bool hasClass(const std::string& name) const;

        // NEW: Inheritance relationship management
        void registerInheritance(const std::string& childName, const std::string& parentName);
        bool wouldCreateCycle(const std::string& childName, const std::string& parentName) const;
        std::vector<std::shared_ptr<ClassDefinition>> getInheritanceChain(const std::string& className) const;
        std::vector<std::string> getChildClasses(const std::string& parentName) const;
        std::string getParentClass(const std::string& childName) const;

    private:
        // Helper method for cycle detection
        bool wouldCreateCycleHelper(const std::string& current, const std::string& target,
                                    std::unordered_set<std::string>& visited, int depth) const;

        static constexpr int MAX_INHERITANCE_DEPTH = 20;
    };
}
