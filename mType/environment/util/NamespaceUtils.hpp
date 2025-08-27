#pragma once
#include <string>
#include <vector>
#include <sstream>

namespace environment::utils
{
    class NamespaceUtils
    {
    public:
        static std::string vectorToPath(const std::vector<std::string>& namespacePath);

        static std::vector<std::string> pathToVector(const std::string& namespacePath);

        static bool isAncestorOf(const std::vector<std::string>& ancestor, const std::vector<std::string>& descendant);
        
        static std::vector<std::string> getCommonAncestor(const std::vector<std::string>& path1, const std::vector<std::string>& path2);
        
    };
}