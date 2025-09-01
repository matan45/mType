#include "ClassRegistry.hpp"
#include <algorithm>
#include <iostream>

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
}
