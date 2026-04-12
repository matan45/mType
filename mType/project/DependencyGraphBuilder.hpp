#pragma once
#include "DependencyGraph.hpp"
#include "ProjectConfig.hpp"
#include "WorkspaceConfig.hpp"
#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>

namespace ast
{
    class ASTNode;

    namespace nodes::statements
    {
        class ImportNode;
    }
}

namespace services
{
    class ImportManager;
}

namespace project
{
    class DependencyGraphBuilder
    {
    public:
        DependencyGraphBuilder();

        // Build graph for a single project
        DependencyGraph build(const ProjectConfig& config);

        // Build graph for a single project with workspace context
        DependencyGraph build(
            const ProjectConfig& config,
            const std::unordered_map<std::string, std::string>& workspaceAliases,
            const std::vector<std::string>& additionalRoots);

        // Build graph for a workspace (union of all member projects)
        DependencyGraph build(const WorkspaceConfig& config);

    private:
        // Walk an AST subtree collecting ImportNode children
        void collectImportNodes(
            ast::ASTNode* node,
            std::vector<ast::nodes::statements::ImportNode*>& imports);

        // Classify a resolved path relative to the project
        NodeKind classifyFile(
            const std::string& resolvedPath,
            const ProjectConfig& config);

        // Make a display-friendly relative path
        std::string makeRelativePath(
            const std::string& absolutePath,
            const std::string& projectRoot);

        // Register a node if not already present
        void ensureNode(
            const std::string& filePath,
            const ProjectConfig& config,
            std::unordered_map<std::string, DependencyNode>& nodes);
    };
}
