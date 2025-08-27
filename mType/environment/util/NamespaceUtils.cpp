#include "NamespaceUtils.hpp"

namespace environment::utils
{
    std::string NamespaceUtils::vectorToPath(const std::vector<std::string>& namespacePath)
    {
        if (namespacePath.empty()) return "";
            
        std::ostringstream oss;
        for (size_t i = 0; i < namespacePath.size(); ++i)
        {
            if (i > 0) oss << "::";
            oss << namespacePath[i];
        }
        return oss.str();
    }

    std::vector<std::string> NamespaceUtils::pathToVector(const std::string& namespacePath)
    {
        if (namespacePath.empty()) return {};
            
        std::vector<std::string> result;
        std::istringstream iss(namespacePath);
        std::string segment;
            
        while (std::getline(iss, segment, ':'))
        {
            if (!segment.empty() && segment != ":")
            {
                result.push_back(segment);
            }
                
            if (iss.peek() == ':')
            {
                iss.ignore();
            }
        }
            
        return result;
    }

    bool NamespaceUtils::isAncestorOf(const std::vector<std::string>& ancestor,
        const std::vector<std::string>& descendant)
    {
        if (ancestor.size() >= descendant.size()) return false;
            
        for (size_t i = 0; i < ancestor.size(); ++i)
        {
            if (ancestor[i] != descendant[i]) return false;
        }
            
        return true;
    }

    std::vector<std::string> NamespaceUtils::getCommonAncestor(const std::vector<std::string>& path1,
        const std::vector<std::string>& path2)
    {
        std::vector<std::string> common;
        size_t minSize = std::min(path1.size(), path2.size());
            
        for (size_t i = 0; i < minSize; ++i)
        {
            if (path1[i] == path2[i])
            {
                common.push_back(path1[i]);
            }
            else
            {
                break;
            }
        }
            
        return common;
    }
}
