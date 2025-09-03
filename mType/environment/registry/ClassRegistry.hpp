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
    };
}
