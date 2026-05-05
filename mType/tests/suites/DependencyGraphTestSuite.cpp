#include "DependencyGraphTestSuite.hpp"
#include "../../project/DependencyGraph.hpp"
#include "../../project/DependencyGraphBuilder.hpp"
#include "../../project/DependencyGraphFormatter.hpp"
#include "../../project/ProjectConfigParser.hpp"
#include "../../project/WorkspaceConfig.hpp"
#include <filesystem>
#include <sstream>
#include <stdexcept>
#include <algorithm>

namespace tests::testSuite
{
    using namespace testFramework;
    namespace fs = std::filesystem;

    // ── Helper: locate test fixtures relative to repo root ──

    static fs::path testFixturesDir()
    {
        return fs::path("mType/tests/testFiles/deps");
    }

    // ── Helper: build a graph from a test directory ──────────

    static project::DependencyGraph buildGraphFromDir(const std::string& subDir)
    {
        fs::path dir = testFixturesDir() / subDir;
        std::string absDir = fs::canonical(dir).string();

        project::ProjectConfig config;
        config.name = "TestProject";
        config.version = "1.0.0";
        config.projectRoot = absDir;

        for (const auto& entry : fs::directory_iterator(absDir))
        {
            if (entry.is_regular_file() && entry.path().extension() == ".mt")
            {
                config.resolvedSourceFiles.push_back(
                    fs::canonical(entry.path()).string());
            }
        }

        std::sort(config.resolvedSourceFiles.begin(),
                  config.resolvedSourceFiles.end());

        project::DependencyGraphBuilder builder;
        return builder.build(config);
    }

    // ── Helper: find canonical path of a node by filename ──

    static std::string findNodeByName(const project::DependencyGraph& graph,
                                      const std::string& filename)
    {
        for (const auto& [path, node] : graph.getNodes())
        {
            if (fs::path(path).filename().string() == filename)
            {
                return path;
            }
        }
        return "";
    }

    void DependencyGraphTestSuite::setupTests()
    {
        // =============================================
        // Core Graph Data Structure Tests
        // =============================================

        addCallbackTest("Graph from simple project has correct node count", "",
            [](services::ScriptAPI&)
        {
            auto graph = buildGraphFromDir("simple");

            // 3 source files: main.mt, helper.mt, standalone.mt
            if (graph.nodeCount() != 3)
                throw std::runtime_error(
                    "Expected 3 nodes, got " + std::to_string(graph.nodeCount()));
        });

        addCallbackTest("Graph from simple project has correct edge count", "",
            [](services::ScriptAPI&)
        {
            auto graph = buildGraphFromDir("simple");

            // main.mt -> helper.mt (1 edge)
            if (graph.edgeCount() != 1)
                throw std::runtime_error(
                    "Expected 1 edge, got " + std::to_string(graph.edgeCount()));
        });

        addCallbackTest("getDependencies returns correct edges", "",
            [](services::ScriptAPI&)
        {
            auto graph = buildGraphFromDir("simple");
            std::string mainPath = findNodeByName(graph, "main.mt");

            if (mainPath.empty())
                throw std::runtime_error("Could not find main.mt in graph");

            auto deps = graph.getDependencies(mainPath);
            if (deps.size() != 1)
                throw std::runtime_error(
                    "Expected 1 dependency for main.mt, got " +
                    std::to_string(deps.size()));

            if (fs::path(deps[0].to).filename().string() != "helper.mt")
                throw std::runtime_error(
                    "Expected dependency on helper.mt, got " + deps[0].to);
        });

        addCallbackTest("getDependents returns correct reverse edges", "",
            [](services::ScriptAPI&)
        {
            auto graph = buildGraphFromDir("simple");
            std::string helperPath = findNodeByName(graph, "helper.mt");

            auto dependents = graph.getDependents(helperPath);
            if (dependents.size() != 1)
                throw std::runtime_error(
                    "Expected 1 dependent for helper.mt, got " +
                    std::to_string(dependents.size()));
        });

        addCallbackTest("Standalone file has no dependencies", "",
            [](services::ScriptAPI&)
        {
            auto graph = buildGraphFromDir("simple");
            std::string standalonePath = findNodeByName(graph, "standalone.mt");

            auto deps = graph.getDependencies(standalonePath);
            if (!deps.empty())
                throw std::runtime_error(
                    "Expected 0 dependencies for standalone.mt, got " +
                    std::to_string(deps.size()));
        });

        addCallbackTest("Entry points are files with no dependents", "",
            [](services::ScriptAPI&)
        {
            auto graph = buildGraphFromDir("simple");
            auto entries = graph.entryPoints();

            // main.mt and standalone.mt have no dependents
            if (entries.size() != 2)
                throw std::runtime_error(
                    "Expected 2 entry points, got " +
                    std::to_string(entries.size()));
        });

        // =============================================
        // Chain Dependency Tests
        // =============================================

        addCallbackTest("Chain: a -> b -> c builds correct graph", "",
            [](services::ScriptAPI&)
        {
            auto graph = buildGraphFromDir("chain");

            if (graph.nodeCount() != 3)
                throw std::runtime_error(
                    "Expected 3 nodes, got " + std::to_string(graph.nodeCount()));

            if (graph.edgeCount() != 2)
                throw std::runtime_error(
                    "Expected 2 edges, got " + std::to_string(graph.edgeCount()));
        });

        addCallbackTest("Chain: transitive dependencies", "",
            [](services::ScriptAPI&)
        {
            auto graph = buildGraphFromDir("chain");
            std::string aPath = findNodeByName(graph, "a.mt");

            auto transitive = graph.transitiveDependencies(aPath);

            // a depends transitively on b and c
            if (transitive.size() != 2)
                throw std::runtime_error(
                    "Expected 2 transitive deps for a.mt, got " +
                    std::to_string(transitive.size()));
        });

        addCallbackTest("Chain: topological order is valid", "",
            [](services::ScriptAPI&)
        {
            auto graph = buildGraphFromDir("chain");
            auto order = graph.topologicalOrder();

            if (order.size() != 3)
                throw std::runtime_error(
                    "Expected topological order with 3 elements, got " +
                    std::to_string(order.size()));

            // a must come before b, b must come before c
            auto posA = std::find(order.begin(), order.end(),
                                  findNodeByName(graph, "a.mt"));
            auto posB = std::find(order.begin(), order.end(),
                                  findNodeByName(graph, "b.mt"));
            auto posC = std::find(order.begin(), order.end(),
                                  findNodeByName(graph, "c.mt"));

            if (posA > posB || posB > posC)
                throw std::runtime_error(
                    "Topological order violated: a should come before b before c");
        });

        addCallbackTest("Chain: findPath from a to c", "",
            [](services::ScriptAPI&)
        {
            auto graph = buildGraphFromDir("chain");
            std::string aPath = findNodeByName(graph, "a.mt");
            std::string cPath = findNodeByName(graph, "c.mt");

            auto path = graph.findPath(aPath, cPath);
            if (path.size() != 3)
                throw std::runtime_error(
                    "Expected path of length 3 (a->b->c), got " +
                    std::to_string(path.size()));
        });

        // =============================================
        // Diamond Dependency Tests
        // =============================================

        addCallbackTest("Diamond: top -> left/right -> bottom", "",
            [](services::ScriptAPI&)
        {
            auto graph = buildGraphFromDir("diamond");

            if (graph.nodeCount() != 4)
                throw std::runtime_error(
                    "Expected 4 nodes, got " + std::to_string(graph.nodeCount()));

            // top->left, top->right, left->bottom, right->bottom = 4 edges
            if (graph.edgeCount() != 4)
                throw std::runtime_error(
                    "Expected 4 edges, got " + std::to_string(graph.edgeCount()));
        });

        addCallbackTest("Diamond: bottom has two dependents", "",
            [](services::ScriptAPI&)
        {
            auto graph = buildGraphFromDir("diamond");
            std::string bottomPath = findNodeByName(graph, "bottom.mt");

            auto dependents = graph.getDependents(bottomPath);
            if (dependents.size() != 2)
                throw std::runtime_error(
                    "Expected 2 dependents for bottom.mt, got " +
                    std::to_string(dependents.size()));
        });

        addCallbackTest("Diamond: no cycles detected", "",
            [](services::ScriptAPI&)
        {
            auto graph = buildGraphFromDir("diamond");
            auto cycles = graph.findCycles();

            if (!cycles.empty())
                throw std::runtime_error(
                    "Expected 0 cycles, got " +
                    std::to_string(cycles.size()));
        });

        addCallbackTest("Diamond: topological order is valid", "",
            [](services::ScriptAPI&)
        {
            auto graph = buildGraphFromDir("diamond");
            auto order = graph.topologicalOrder();

            if (order.size() != 4)
                throw std::runtime_error(
                    "Expected 4 in topological order, got " +
                    std::to_string(order.size()));

            // top must come before left and right; both must come before bottom
            auto posTop = std::find(order.begin(), order.end(),
                                    findNodeByName(graph, "top.mt"));
            auto posBottom = std::find(order.begin(), order.end(),
                                       findNodeByName(graph, "bottom.mt"));

            if (posTop > posBottom)
                throw std::runtime_error(
                    "Topological order violated: top should come before bottom");
        });

        // =============================================
        // Selective vs Wildcard Import Tests
        // =============================================

        addCallbackTest("Selective import records symbols", "",
            [](services::ScriptAPI&)
        {
            auto graph = buildGraphFromDir("simple");
            std::string mainPath = findNodeByName(graph, "main.mt");

            auto deps = graph.getDependencies(mainPath);
            if (deps.empty())
                throw std::runtime_error("main.mt has no dependencies");

            // main.mt uses selective import: import { Helper } from "helper.mt"
            if (deps[0].isWildcard)
                throw std::runtime_error("Expected selective import, got wildcard");

            if (deps[0].symbols.empty())
                throw std::runtime_error("Expected symbols in selective import");

            bool foundHelper = false;
            for (const auto& sym : deps[0].symbols)
            {
                if (sym == "Helper") foundHelper = true;
            }
            if (!foundHelper)
                throw std::runtime_error(
                    "Expected 'Helper' in imported symbols");
        });

        // =============================================
        // Formatter Tests
        // =============================================

        addCallbackTest("Tree formatter produces output", "",
            [](services::ScriptAPI&)
        {
            auto graph = buildGraphFromDir("simple");
            std::ostringstream out;
            project::DependencyGraphFormatter::renderTree(graph, out, false);

            std::string result = out.str();
            if (result.empty())
                throw std::runtime_error("Tree formatter produced empty output");

            if (result.find("TestProject") == std::string::npos)
                throw std::runtime_error(
                    "Tree output should contain project name");
        });

        addCallbackTest("DOT formatter produces valid output", "",
            [](services::ScriptAPI&)
        {
            auto graph = buildGraphFromDir("simple");
            std::ostringstream out;
            project::DependencyGraphFormatter::renderDot(graph, out);

            std::string result = out.str();
            if (result.find("digraph") == std::string::npos)
                throw std::runtime_error("DOT output missing 'digraph' header");

            if (result.find("->") == std::string::npos)
                throw std::runtime_error("DOT output missing edges");
        });

        addCallbackTest("JSON formatter produces valid output", "",
            [](services::ScriptAPI&)
        {
            auto graph = buildGraphFromDir("simple");
            auto json = project::DependencyGraphFormatter::toJson(graph);

            if (!json->isObject())
                throw std::runtime_error("JSON output should be an object");

            if (!json->hasProperty("project"))
                throw std::runtime_error("JSON missing 'project' field");

            if (!json->hasProperty("nodes"))
                throw std::runtime_error("JSON missing 'nodes' field");

            if (!json->hasProperty("edges"))
                throw std::runtime_error("JSON missing 'edges' field");

            if (!json->hasProperty("stats"))
                throw std::runtime_error("JSON missing 'stats' field");

            auto stats = json->getProperty("stats");
            if (stats->getProperty("nodeCount")->asInt() != 3)
                throw std::runtime_error("JSON stats nodeCount should be 3");
        });

        addCallbackTest("Cycle report shows no cycles for acyclic graph", "",
            [](services::ScriptAPI&)
        {
            auto graph = buildGraphFromDir("chain");
            auto cycles = graph.findCycles();

            std::ostringstream out;
            project::DependencyGraphFormatter::renderCycles(
                cycles, graph.getProjectRoot(), out, false);

            std::string result = out.str();
            if (result.find("No circular dependencies") == std::string::npos)
                throw std::runtime_error(
                    "Expected 'No circular dependencies' message");
        });

        addCallbackTest("Why formatter finds import chain", "",
            [](services::ScriptAPI&)
        {
            auto graph = buildGraphFromDir("chain");

            std::ostringstream out;
            project::DependencyGraphFormatter::renderWhy(
                graph, "c.mt", out, false);

            std::string result = out.str();
            if (result.find("c.mt") == std::string::npos)
                throw std::runtime_error(
                    "Why output should reference target file");
        });

        // #13 — renderWhy reports error for non-existent file
        addCallbackTest("Why formatter shows error for missing file", "",
            [](services::ScriptAPI&)
        {
            auto graph = buildGraphFromDir("simple");

            std::ostringstream out;
            project::DependencyGraphFormatter::renderWhy(
                graph, "nonexistent.mt", out, false);

            std::string result = out.str();
            if (result.find("File not found") == std::string::npos)
                throw std::runtime_error(
                    "Expected 'File not found' message for missing file");
        });

        // =============================================
        // Edge Cases
        // =============================================

        addCallbackTest("Empty graph has zero nodes and edges", "",
            [](services::ScriptAPI&)
        {
            project::ProjectConfig config;
            config.name = "EmptyProject";
            config.projectRoot = fs::current_path().string();

            project::DependencyGraphBuilder builder;
            auto graph = builder.build(config);

            if (graph.nodeCount() != 0)
                throw std::runtime_error(
                    "Expected 0 nodes, got " + std::to_string(graph.nodeCount()));
            if (graph.edgeCount() != 0)
                throw std::runtime_error(
                    "Expected 0 edges, got " + std::to_string(graph.edgeCount()));
        });

        addCallbackTest("findPath returns empty for unreachable node", "",
            [](services::ScriptAPI&)
        {
            auto graph = buildGraphFromDir("simple");
            std::string mainPath = findNodeByName(graph, "main.mt");
            std::string standalonePath = findNodeByName(graph, "standalone.mt");

            auto path = graph.findPath(mainPath, standalonePath);
            if (!path.empty())
                throw std::runtime_error(
                    "Expected empty path between unconnected nodes");
        });

        addCallbackTest("hasNode returns correct results", "",
            [](services::ScriptAPI&)
        {
            auto graph = buildGraphFromDir("simple");
            std::string mainPath = findNodeByName(graph, "main.mt");

            if (!graph.hasNode(mainPath))
                throw std::runtime_error("hasNode should return true for main.mt");

            if (graph.hasNode("nonexistent.mt"))
                throw std::runtime_error(
                    "hasNode should return false for nonexistent file");
        });

        // =============================================
        // Workspace Graph Builder Test (#12)
        // =============================================

        addCallbackTest("Workspace graph merges member projects", "",
            [](services::ScriptAPI&)
        {
            fs::path fixturesDir = testFixturesDir();

            // Build two ProjectConfigs from the simple and chain directories
            fs::path simpleDir = fs::canonical(fixturesDir / "simple");
            fs::path chainDir = fs::canonical(fixturesDir / "chain");

            project::ProjectConfig simpleConfig;
            simpleConfig.name = "simple";
            simpleConfig.projectRoot = simpleDir.string();
            for (const auto& entry : fs::directory_iterator(simpleDir))
            {
                if (entry.is_regular_file() && entry.path().extension() == ".mt")
                    simpleConfig.resolvedSourceFiles.push_back(
                        fs::canonical(entry.path()).string());
            }
            std::sort(simpleConfig.resolvedSourceFiles.begin(),
                      simpleConfig.resolvedSourceFiles.end());

            project::ProjectConfig chainConfig;
            chainConfig.name = "chain";
            chainConfig.projectRoot = chainDir.string();
            for (const auto& entry : fs::directory_iterator(chainDir))
            {
                if (entry.is_regular_file() && entry.path().extension() == ".mt")
                    chainConfig.resolvedSourceFiles.push_back(
                        fs::canonical(entry.path()).string());
            }
            std::sort(chainConfig.resolvedSourceFiles.begin(),
                      chainConfig.resolvedSourceFiles.end());

            // Build a WorkspaceConfig with both as members
            project::WorkspaceConfig wsConfig;
            wsConfig.name = "TestWorkspace";
            wsConfig.workspaceRoot = fixturesDir.string();

            project::MemberProject simpleMember;
            simpleMember.path = simpleDir.string();
            simpleMember.config = std::make_unique<project::ProjectConfig>(simpleConfig);
            wsConfig.members.push_back(std::move(simpleMember));

            project::MemberProject chainMember;
            chainMember.path = chainDir.string();
            chainMember.config = std::make_unique<project::ProjectConfig>(chainConfig);
            wsConfig.members.push_back(std::move(chainMember));

            project::DependencyGraphBuilder builder;
            auto graph = builder.build(wsConfig);

            // simple has 3 nodes + 1 edge, chain has 3 nodes + 2 edges = 6 nodes, 3 edges
            if (graph.nodeCount() != 6)
                throw std::runtime_error(
                    "Expected 6 nodes in workspace graph, got " +
                    std::to_string(graph.nodeCount()));

            if (graph.edgeCount() != 3)
                throw std::runtime_error(
                    "Expected 3 edges in workspace graph, got " +
                    std::to_string(graph.edgeCount()));
        });
    }
}
