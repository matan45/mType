#include "DependencyGraphBuilder.hpp"
#include "ImportManagerConfig.hpp"
#include "../services/ImportManager.hpp"
#include "../ast/ASTNode.hpp"
#include "../ast/nodes/statements/ImportNode.hpp"
#include "../ast/nodes/statements/ProgramNode.hpp"
#include "../ast/nodes/statements/BlockNode.hpp"
#include "../lexer/Lexer.hpp"
#include "../parser/Parser.hpp"
#include <filesystem>
#include <fstream>
#include <stdexcept>

#ifdef _WIN32
#include <process.h>
#define getpid _getpid
#else
#include <unistd.h>
#endif

namespace project
{
    namespace fs = std::filesystem;

    DependencyGraphBuilder::DependencyGraphBuilder() = default;

    DependencyGraph DependencyGraphBuilder::build(const ProjectConfig& config)
    {
        return build(config, {}, {});
    }

    DependencyGraph DependencyGraphBuilder::build(
        const ProjectConfig& config,
        const std::unordered_map<std::string, std::string>& workspaceAliases,
        const std::vector<std::string>& additionalRoots)
    {
        // Build a virtual source that imports all project files (same
        // technique as ProjectBuilder::compileToProgram). This lets
        // ImportManager::resolveAllImports do the full DFS resolution
        // for us — the exact same code path used during compilation.
        std::string virtualSource;
        for (const auto& sourceFile : config.resolvedSourceFiles)
        {
            fs::path srcPath(sourceFile);
            fs::path projRoot(config.projectRoot);
            fs::path relativePath = fs::relative(srcPath, projRoot);
            virtualSource += "import * from \"" + relativePath.generic_string() + "\";\n";
        }

        // Write virtual source to a collision-safe temp file (PID suffix)
        fs::path tempDir = fs::temp_directory_path();
        std::string tempName = "_mtype_deps_" + config.name + "_"
                             + std::to_string(getpid()) + ".mt";
        fs::path tempFile = tempDir / tempName;

        struct TempFileGuard {
            fs::path path;
            ~TempFileGuard() { fs::remove(path); }
        } guard{tempFile};

        {
            std::ofstream out(tempFile);
            if (!out)
            {
                throw std::runtime_error(
                    "Could not create temp file: " + tempFile.string());
            }
            out << virtualSource;
        }

        // Configure ImportManager with project + workspace context
        services::ImportManager importManager;
        configureImportManager(importManager, config,
                               workspaceAliases, additionalRoots);
        importManager.setCurrentFilePath(tempFile.string());

        // Allow imports from temp dir
        importManager.addAllowedRoot(tempDir.string());

        // Parse and resolve ALL imports recursively (proven code path)
        lexer::Lexer lex(tempFile.string());
        parser::Parser par(lex, nullptr);
        auto rootAST = par.parseProgram();
        importManager.resolveAllImports(rootAST.get());

        // Now walk the resolved AST tree to extract edges.
        std::unordered_map<std::string, DependencyNode> nodes;
        std::unordered_map<std::string, std::vector<DependencyEdge>> adjacency;

        // Canonical path of the temp file — exclude it from the graph
        std::string tempCanonical = fs::canonical(tempFile).string();

        // Build a reverse map: AST pointer -> file path
        auto importedFiles = importManager.getImportedFiles();
        std::unordered_map<ast::ASTNode*, std::string> astToFile;
        for (const auto& file : importedFiles)
        {
            ast::ASTNode* cachedAST = importManager.parseAndCacheAST(file);
            if (cachedAST)
            {
                astToFile[cachedAST] = file;
            }
        }

        // Walk every cached file's AST and extract import edges
        for (const auto& file : importedFiles)
        {
            std::string canonicalFile;
            try
            {
                canonicalFile = fs::canonical(file).string();
            }
            catch (const std::exception&)
            {
                canonicalFile = file;
            }

            // Skip the synthetic temp file
            if (canonicalFile == tempCanonical) continue;

            ensureNode(canonicalFile, config, nodes);

            ast::ASTNode* fileAST = importManager.parseAndCacheAST(file);
            if (!fileAST) continue;

            std::vector<ast::nodes::statements::ImportNode*> imports;
            collectImportNodes(fileAST, imports);

            for (auto* importNode : imports)
            {
                if (!importNode->isResolved() || !importNode->getImportedAST())
                    continue;

                auto it = astToFile.find(importNode->getImportedAST());
                if (it == astToFile.end()) continue;

                std::string targetCanonical;
                try
                {
                    targetCanonical = fs::canonical(it->second).string();
                }
                catch (const std::exception&)
                {
                    targetCanonical = it->second;
                }

                ensureNode(targetCanonical, config, nodes);

                DependencyEdge edge;
                edge.from = canonicalFile;
                edge.to = targetCanonical;
                edge.isWildcard = importNode->isWildcard();
                if (!edge.isWildcard)
                {
                    edge.symbols = importNode->getImportedSymbols();
                }
                adjacency[canonicalFile].push_back(std::move(edge));
            }
        }

        return DependencyGraph(
            std::move(nodes),
            std::move(adjacency),
            config.projectRoot,
            config.name);
    }

    DependencyGraph DependencyGraphBuilder::build(const WorkspaceConfig& config)
    {
        std::unordered_map<std::string, DependencyNode> allNodes;
        std::unordered_map<std::string, std::vector<DependencyEdge>> allAdjacency;

        // Build workspace aliases and additional roots
        std::unordered_map<std::string, std::string> workspaceAliases;
        std::vector<std::string> additionalRoots;
        for (const auto& member : config.members)
        {
            if (member.config)
            {
                std::string name = member.getName();
                if (!name.empty())
                {
                    workspaceAliases["@" + name] = member.config->projectRoot;
                }
                additionalRoots.push_back(member.config->projectRoot);
            }
        }

        // Process each member project with workspace context
        for (const auto& member : config.members)
        {
            if (!member.config) continue;

            auto memberGraph = build(*member.config,
                                     workspaceAliases, additionalRoots);

            for (const auto& [path, node] : memberGraph.getNodes())
            {
                allNodes[path] = node;
            }
            for (const auto& [path, _] : memberGraph.getNodes())
            {
                auto deps = memberGraph.getDependencies(path);
                if (!deps.empty())
                {
                    auto& vec = allAdjacency[path];
                    vec.insert(vec.end(), deps.begin(), deps.end());
                }
            }
        }

        return DependencyGraph(
            std::move(allNodes),
            std::move(allAdjacency),
            config.workspaceRoot,
            config.name);
    }

    void DependencyGraphBuilder::collectImportNodes(
        ast::ASTNode* node,
        std::vector<ast::nodes::statements::ImportNode*>& imports)
    {
        if (!node) return;

        if (auto* importNode = dynamic_cast<ast::nodes::statements::ImportNode*>(node))
        {
            imports.push_back(importNode);
            return;
        }

        if (auto* programNode = dynamic_cast<ast::nodes::statements::ProgramNode*>(node))
        {
            for (const auto& stmt : programNode->getStatements())
            {
                collectImportNodes(stmt.get(), imports);
            }
        }
        else if (auto* blockNode = dynamic_cast<ast::nodes::statements::BlockNode*>(node))
        {
            for (const auto& stmt : blockNode->getStatements())
            {
                collectImportNodes(stmt.get(), imports);
            }
        }
    }

    NodeKind DependencyGraphBuilder::classifyFile(
        const std::string& resolvedPath,
        const ProjectConfig& config)
    {
        try
        {
            fs::path canonical = fs::canonical(resolvedPath);
            fs::path root = fs::canonical(config.projectRoot);

            auto [rootEnd, _] = std::mismatch(
                root.begin(), root.end(), canonical.begin(), canonical.end());

            if (rootEnd == root.end())
            {
                return NodeKind::SOURCE_FILE;
            }
        }
        catch (const std::exception&)
        {
        }

        return NodeKind::EXTERNAL_FILE;
    }

    std::string DependencyGraphBuilder::makeRelativePath(
        const std::string& absolutePath,
        const std::string& projectRoot)
    {
        try
        {
            return fs::relative(absolutePath, projectRoot).string();
        }
        catch (const std::exception&)
        {
            return absolutePath;
        }
    }

    void DependencyGraphBuilder::ensureNode(
        const std::string& filePath,
        const ProjectConfig& config,
        std::unordered_map<std::string, DependencyNode>& nodes)
    {
        if (nodes.count(filePath)) return;

        DependencyNode node;
        node.filePath = filePath;
        node.relativePath = makeRelativePath(filePath, config.projectRoot);
        node.kind = classifyFile(filePath, config);
        nodes[filePath] = std::move(node);
    }
}
