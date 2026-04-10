#include "InheritanceTracker.hpp"

namespace environment::registry
{
    void InheritanceTracker::setFindClassCallback(std::function<std::shared_ptr<ClassDefinition>(const std::string&)> callback)
    {
        findClassCallback = callback;
    }

    void InheritanceTracker::registerInheritance(const std::string& childName, const std::string& parentName)
    {
        // Update parent->children mapping (set automatically prevents duplicates)
        parentToChildren[parentName].insert(childName);

        // Update child->parent mapping
        childToParent[childName] = parentName;

        // Invalidate cache for affected classes
        invalidateCache(childName);
    }

    bool InheritanceTracker::wouldCreateCycle(const std::string& childName, const std::string& parentName) const
    {
        std::unordered_set<std::string> visited;
        return wouldCreateCycleHelper(parentName, childName, visited, 0);
    }

    bool InheritanceTracker::wouldCreateCycleHelper(const std::string& current, const std::string& target,
                                                    std::unordered_set<std::string>& visited, int depth) const
    {
        if (depth > MAX_INHERITANCE_DEPTH)
        {
            return true;
        }

        if (current == target)
        {
            return true;
        }

        if (visited.find(current) != visited.end())
        {
            return false;
        }

        visited.insert(current);

        auto it = childToParent.find(current);
        if (it != childToParent.end())
        {
            return wouldCreateCycleHelper(it->second, target, visited, depth + 1);
        }

        return false;
    }

    std::vector<std::shared_ptr<ClassDefinition>> InheritanceTracker::getInheritanceChain(const std::string& className) const
    {
        // Check cache first
        auto cacheIt = inheritanceChainCache.find(className);
        if (cacheIt != inheritanceChainCache.end())
        {
            return cacheIt->second;
        }

        // Compute inheritance chain
        std::vector<std::shared_ptr<ClassDefinition>> chain;

        auto classIt = childToParent.find(className);
        int depth = 0;

        while (classIt != childToParent.end() && depth < MAX_INHERITANCE_DEPTH)
        {
            if (findClassCallback)
            {
                auto parentClass = findClassCallback(classIt->second);
                if (!parentClass)
                {
                    break;
                }
                chain.push_back(parentClass);
            }

            classIt = childToParent.find(classIt->second);
            depth++;
        }

        // Cache the result
        inheritanceChainCache[className] = chain;

        return chain;
    }

    std::vector<std::string> InheritanceTracker::getChildClasses(const std::string& parentName) const
    {
        auto it = parentToChildren.find(parentName);
        if (it != parentToChildren.end())
        {
            // Convert unordered_set to vector
            return std::vector<std::string>(it->second.begin(), it->second.end());
        }
        return {};
    }

    std::string InheritanceTracker::getParentClass(const std::string& childName) const
    {
        auto it = childToParent.find(childName);
        if (it != childToParent.end())
        {
            return it->second;
        }
        return "";
    }

    void InheritanceTracker::invalidateCache(const std::string& className)
    {
        // Use iterative breadth-first traversal to avoid stack overflow
        // with deep class hierarchies
        std::queue<std::string> toInvalidate;
        toInvalidate.push(className);

        while (!toInvalidate.empty())
        {
            std::string current = toInvalidate.front();
            toInvalidate.pop();

            // Invalidate the current class's cache
            inheritanceChainCache.erase(current);

            // Add all direct children to the queue
            auto children = getChildClasses(current);
            for (const auto& child : children)
            {
                toInvalidate.push(child);
            }
        }
    }

    void InheritanceTracker::clearCache()
    {
        inheritanceChainCache.clear();
    }

    void InheritanceTracker::clear()
    {
        parentToChildren.clear();
        childToParent.clear();
        inheritanceChainCache.clear();
    }
}
