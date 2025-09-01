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

    public:
        explicit ClassRegistry() = default;
        ~ClassRegistry() override = default;

        void registerItem(const std::string& name, std::shared_ptr<ClassDefinition> item) override;
        std::shared_ptr<ClassDefinition> findItem(const std::string& name) const override;
        bool hasItem(const std::string& name) const override;
        void removeItem(const std::string& name) override;
        std::vector<std::string> getAllItemNames() const override;
        size_t getItemCount() const override;
        
        std::string getComponentName() const override;
        
        void registerClass(const std::string& name, std::shared_ptr<ClassDefinition> classDefinition);
        std::shared_ptr<ClassDefinition> findClass(const std::string& name) const;
        bool hasClass(const std::string& name) const;
    };
}
