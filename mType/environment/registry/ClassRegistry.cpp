#include "ClassRegistry.hpp"
#include <algorithm>
#include <sstream>

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

    std::shared_ptr<ClassDefinition> ClassRegistry::findQualifiedItem(const std::vector<std::string>& qualifiedName) const
    {
        if (qualifiedName.empty()) return nullptr;
        
        if (qualifiedName.size() == 1)
        {
            return findClass(qualifiedName[0]);
        }
        
        std::vector<std::string> namespacePath(qualifiedName.begin(), qualifiedName.end() - 1);
        return findClassInNamespace(namespacePath, qualifiedName.back());
    }

    bool ClassRegistry::hasItem(const std::string& name) const
    {
        return hasClass(name);
    }

    void ClassRegistry::removeItem(const std::string& name)
    {
        classes.erase(name);
        
        for (auto& [ns, nsClasses] : namespacedClasses)
        {
            nsClasses.erase(name);
        }
    }

    std::vector<std::string> ClassRegistry::getAllItemNames() const
    {
        std::vector<std::string> names;
        
        for (const auto& [name, _] : classes)
        {
            names.push_back(name);
        }
        
        for (const auto& [ns, nsClasses] : namespacedClasses)
        {
            for (const auto& [className, _] : nsClasses)
            {
                names.push_back(ns + "::" + className);
            }
        }
        
        std::sort(names.begin(), names.end());
        return names;
    }

    size_t ClassRegistry::getItemCount() const
    {
        size_t count = classes.size();
        for (const auto& [ns, nsClasses] : namespacedClasses)
        {
            count += nsClasses.size();
        }
        return count;
    }

    std::string ClassRegistry::getComponentName() const
    {
        return "ClassRegistry";
    }

    void ClassRegistry::registerClass(const std::string& name, std::shared_ptr<ClassDefinition> classDefinition)
    {
        classes[name] = classDefinition;
    }

    void ClassRegistry::registerClassInNamespace(const std::vector<std::string>& namespacePath, const std::string& className, std::shared_ptr<ClassDefinition> classDefinition)
    {
        std::string nsPath;
        if (!namespacePath.empty())
        {
            std::ostringstream oss;
            for (size_t i = 0; i < namespacePath.size(); ++i)
            {
                if (i > 0) oss << "::";
                oss << namespacePath[i];
            }
            nsPath = oss.str();
        }
        
        namespacedClasses[nsPath][className] = classDefinition;
        classDefinition->setNamespaceContext(namespacePath);
    }

    std::shared_ptr<ClassDefinition> ClassRegistry::findClass(const std::string& name) const
    {
        auto it = classes.find(name);
        return (it != classes.end()) ? it->second : nullptr;
    }

    std::shared_ptr<ClassDefinition> ClassRegistry::findClassInNamespace(const std::vector<std::string>& namespacePath, const std::string& className) const
    {
        std::string nsPath;
        if (!namespacePath.empty())
        {
            std::ostringstream oss;
            for (size_t i = 0; i < namespacePath.size(); ++i)
            {
                if (i > 0) oss << "::";
                oss << namespacePath[i];
            }
            nsPath = oss.str();
        }
        
        auto nsIt = namespacedClasses.find(nsPath);
        if (nsIt != namespacedClasses.end())
        {
            auto classIt = nsIt->second.find(className);
            if (classIt != nsIt->second.end())
            {
                return classIt->second;
            }
        }
        
        return findClass(className);
    }

    bool ClassRegistry::hasClass(const std::string& name) const
    {
        return classes.find(name) != classes.end();
    }

    std::vector<std::string> ClassRegistry::getClassesInNamespace(const std::vector<std::string>& namespacePath) const
    {
        std::vector<std::string> classNames;
        
        std::string nsPath;
        if (!namespacePath.empty())
        {
            std::ostringstream oss;
            for (size_t i = 0; i < namespacePath.size(); ++i)
            {
                if (i > 0) oss << "::";
                oss << namespacePath[i];
            }
            nsPath = oss.str();
        }
        
        auto nsIt = namespacedClasses.find(nsPath);
        if (nsIt != namespacedClasses.end())
        {
            for (const auto& [className, _] : nsIt->second)
            {
                classNames.push_back(className);
            }
        }
        
        std::sort(classNames.begin(), classNames.end());
        return classNames;
    }
}
