#include "DependencyGraphFormatter.hpp"
#include <cstddef>
#include <cstdint>
#include "../diagnostics/AnsiStyle.hpp"
#include <filesystem>
#include <algorithm>
#include <sstream>

namespace project
{
    // ── Helpers ──────────────────────────────────────────────

    std::string DependencyGraphFormatter::displayPath(
        const std::string& absolutePath,
        const std::string& projectRoot)
    {
        try
        {
            auto rel = std::filesystem::relative(absolutePath, projectRoot);
            std::string result = rel.generic_string();
            return result.empty() ? absolutePath : result;
        }
        catch (const std::exception&)
        {
            return absolutePath;
        }
    }

    std::string DependencyGraphFormatter::colorize(
        const std::string& text,
        std::string_view color,
        bool enabled)
    {
        if (!enabled) return text;
        return std::string(color) + text + std::string(diagnostics::ansi::RESET);
    }

    std::string DependencyGraphFormatter::nodeColor(
        NodeKind kind,
        bool enabled)
    {
        if (!enabled) return "";
        switch (kind)
        {
            case NodeKind::SOURCE_FILE:  return std::string(diagnostics::ansi::GREEN);
            case NodeKind::EXTERNAL_FILE: return std::string(diagnostics::ansi::CYAN);
            default: return "";
        }
    }

    // ── Terminal Tree ────────────────────────────────────────

    void DependencyGraphFormatter::renderTree(
        const DependencyGraph& graph,
        std::ostream& out,
        bool colorEnabled)
    {
        out << colorize(graph.getProjectName(), diagnostics::ansi::BOLD, colorEnabled)
            << " (" << graph.nodeCount() << " files, "
            << graph.edgeCount() << " edges)\n";

        auto entries = graph.entryPoints();
        std::unordered_set<std::string> visited;

        for (size_t i = 0; i < entries.size(); ++i)
        {
            bool isLast = (i == entries.size() - 1);
            renderTreeNode(graph, entries[i], out, colorEnabled,
                          "", isLast, visited);
        }
    }

    void DependencyGraphFormatter::renderTreeNode(
        const DependencyGraph& graph,
        const std::string& file,
        std::ostream& out,
        bool colorEnabled,
        const std::string& prefix,
        bool isLast,
        std::unordered_set<std::string>& visited)
    {
        std::string connector = isLast ? "\xe2\x94\x94\xe2\x94\x80\xe2\x94\x80 "
                                       : "\xe2\x94\x9c\xe2\x94\x80\xe2\x94\x80 ";
        std::string display = displayPath(file, graph.getProjectRoot());

        const auto& nodes = graph.getNodes();
        auto nodeIt = nodes.find(file);
        NodeKind kind = (nodeIt != nodes.end()) ? nodeIt->second.kind : NodeKind::SOURCE_FILE;

        bool alreadyVisited = visited.count(file) > 0;

        out << prefix << connector
            << colorize(display, nodeColor(kind, true), colorEnabled);

        if (alreadyVisited)
        {
            out << colorize("  (*)", diagnostics::ansi::DIM, colorEnabled);
        }
        out << "\n";

        if (alreadyVisited) return;
        visited.insert(file);

        auto deps = graph.getDependencies(file);
        std::sort(deps.begin(), deps.end(),
                  [](const DependencyEdge& a, const DependencyEdge& b)
                  { return a.to < b.to; });

        std::string childPrefix = prefix + (isLast ? "    " : "\xe2\x94\x82   ");

        for (size_t i = 0; i < deps.size(); ++i)
        {
            bool childIsLast = (i == deps.size() - 1);
            renderTreeNode(graph, deps[i].to, out, colorEnabled,
                          childPrefix, childIsLast, visited);
        }
    }

    // ── DOT Format ───────────────────────────────────────────

    void DependencyGraphFormatter::renderDot(
        const DependencyGraph& graph,
        std::ostream& out)
    {
        out << "digraph \"" << graph.getProjectName() << "\" {\n";
        out << "  rankdir=LR;\n";
        out << "  node [shape=box, style=filled, fontname=\"Helvetica\"];\n\n";

        // Emit nodes with colors based on kind
        for (const auto& [path, node] : graph.getNodes())
        {
            std::string display = displayPath(path, graph.getProjectRoot());
            std::string fillColor;
            std::string style = "filled";

            switch (node.kind)
            {
                case NodeKind::SOURCE_FILE:
                    fillColor = "#90EE90"; // light green
                    break;
                case NodeKind::EXTERNAL_FILE:
                    fillColor = "#87CEEB"; // light blue
                    style = "filled,dashed";
                    break;
            }

            out << "  \"" << display << "\" [fillcolor=\""
                << fillColor << "\", style=\"" << style << "\"];\n";
        }

        out << "\n";

        // Emit edges
        for (const auto& [source, edges] : graph.getNodes())
        {
            std::string srcDisplay = displayPath(source, graph.getProjectRoot());

            for (const auto& edge : graph.getDependencies(source))
            {
                std::string dstDisplay = displayPath(edge.to, graph.getProjectRoot());
                out << "  \"" << srcDisplay << "\" -> \"" << dstDisplay << "\"";

                if (!edge.isWildcard && !edge.symbols.empty())
                {
                    out << " [label=\"{";
                    for (size_t i = 0; i < edge.symbols.size(); ++i)
                    {
                        if (i > 0) out << ", ";
                        out << edge.symbols[i];
                    }
                    out << "}\"]";
                }
                out << ";\n";
            }
        }

        out << "}\n";
    }

    // ── JSON Format ──────────────────────────────────────────

    std::shared_ptr<json::JsonValue> DependencyGraphFormatter::toJson(
        const DependencyGraph& graph)
    {
        auto root = json::JsonValue::object();
        root->setProperty("project", json::JsonValue::string(graph.getProjectName()));
        root->setProperty("root", json::JsonValue::string(graph.getProjectRoot()));

        // Nodes array
        auto nodesArr = json::JsonValue::array();
        for (const auto& [path, node] : graph.getNodes())
        {
            auto jsonNode = json::JsonValue::object();
            jsonNode->setProperty("path",
                json::JsonValue::string(node.relativePath));
            jsonNode->setProperty("absolutePath",
                json::JsonValue::string(node.filePath));

            std::string kindStr;
            switch (node.kind)
            {
                case NodeKind::SOURCE_FILE:  kindStr = "source"; break;
                case NodeKind::EXTERNAL_FILE: kindStr = "external"; break;
            }
            jsonNode->setProperty("kind", json::JsonValue::string(kindStr));
            nodesArr->addToArray(std::move(jsonNode));
        }
        root->setProperty("nodes", std::move(nodesArr));

        // Edges array
        auto edgesArr = json::JsonValue::array();
        for (const auto& [source, _] : graph.getNodes())
        {
            for (const auto& edge : graph.getDependencies(source))
            {
                auto jsonEdge = json::JsonValue::object();
                jsonEdge->setProperty("from",
                    json::JsonValue::string(
                        displayPath(edge.from, graph.getProjectRoot())));
                jsonEdge->setProperty("to",
                    json::JsonValue::string(
                        displayPath(edge.to, graph.getProjectRoot())));
                jsonEdge->setProperty("wildcard",
                    json::JsonValue::boolean(edge.isWildcard));

                auto symbolsArr = json::JsonValue::array();
                for (const auto& sym : edge.symbols)
                {
                    symbolsArr->addToArray(json::JsonValue::string(sym));
                }
                jsonEdge->setProperty("symbols", std::move(symbolsArr));
                edgesArr->addToArray(std::move(jsonEdge));
            }
        }
        root->setProperty("edges", std::move(edgesArr));

        // Stats — compute cycles once to avoid redundant O(V+E) pass
        auto cycles = graph.findCycles();
        auto stats = json::JsonValue::object();
        stats->setProperty("nodeCount",
            json::JsonValue::integer(static_cast<int64_t>(graph.nodeCount())));
        stats->setProperty("edgeCount",
            json::JsonValue::integer(static_cast<int64_t>(graph.edgeCount())));
        stats->setProperty("cycles",
            json::JsonValue::integer(static_cast<int64_t>(cycles.size())));
        root->setProperty("stats", std::move(stats));

        return root;
    }

    // ── Cycle Report ─────────────────────────────────────────

    void DependencyGraphFormatter::renderCycles(
        const std::vector<std::vector<std::string>>& cycles,
        const std::string& projectRoot,
        std::ostream& out,
        bool colorEnabled)
    {
        if (cycles.empty())
        {
            out << colorize("No circular dependencies detected.",
                           diagnostics::ansi::GREEN, colorEnabled) << "\n";
            return;
        }

        out << colorize("Circular dependencies detected: ",
                       diagnostics::ansi::RED, colorEnabled)
            << cycles.size() << " cycle(s)\n\n";

        for (size_t i = 0; i < cycles.size(); ++i)
        {
            out << "  " << colorize("Cycle " + std::to_string(i + 1) + ":",
                                   diagnostics::ansi::BOLD, colorEnabled) << "\n";

            for (size_t j = 0; j < cycles[i].size(); ++j)
            {
                std::string display = displayPath(cycles[i][j], projectRoot);
                out << "    ";
                if (j > 0)
                {
                    out << colorize("-> ", diagnostics::ansi::RED, colorEnabled);
                }
                out << colorize(display, diagnostics::ansi::YELLOW, colorEnabled) << "\n";
            }

            // Show closing edge back to first node
            std::string firstDisplay = displayPath(cycles[i][0], projectRoot);
            out << "    " << colorize("-> ", diagnostics::ansi::RED, colorEnabled)
                << colorize(firstDisplay, diagnostics::ansi::YELLOW, colorEnabled)
                << "\n\n";
        }
    }

    // ── Why Report ───────────────────────────────────────────

    void DependencyGraphFormatter::renderWhy(
        const DependencyGraph& graph,
        const std::string& targetFile,
        std::ostream& out,
        bool colorEnabled)
    {
        // Resolve the target to a node in the graph
        std::string resolvedTarget;
        for (const auto& [path, node] : graph.getNodes())
        {
            if (path == targetFile || node.relativePath == targetFile)
            {
                resolvedTarget = path;
                break;
            }
        }

        if (resolvedTarget.empty())
        {
            // Try partial match on filename
            for (const auto& [path, node] : graph.getNodes())
            {
                std::string filename = std::filesystem::path(path).filename().string();
                if (filename == targetFile ||
                    node.relativePath.find(targetFile) != std::string::npos)
                {
                    resolvedTarget = path;
                    break;
                }
            }
        }

        if (resolvedTarget.empty())
        {
            out << colorize("File not found in dependency graph: ",
                           diagnostics::ansi::RED, colorEnabled)
                << targetFile << "\n";
            return;
        }

        std::string targetDisplay = displayPath(resolvedTarget, graph.getProjectRoot());
        out << "Import chains to "
            << colorize(targetDisplay, diagnostics::ansi::BOLD, colorEnabled)
            << ":\n\n";

        auto entries = graph.entryPoints();
        bool foundPath = false;

        for (const auto& entry : entries)
        {
            auto path = graph.findPath(entry, resolvedTarget);
            if (path.empty()) continue;

            foundPath = true;
            for (size_t i = 0; i < path.size(); ++i)
            {
                std::string display = displayPath(path[i], graph.getProjectRoot());
                std::string indent(i * 4, ' ');

                if (i == 0)
                {
                    out << "  " << colorize(display, diagnostics::ansi::GREEN, colorEnabled) << "\n";
                }
                else
                {
                    std::string connector = (i == path.size() - 1)
                        ? "\xe2\x94\x94\xe2\x94\x80\xe2\x94\x80 "
                        : "\xe2\x94\x9c\xe2\x94\x80\xe2\x94\x80 ";
                    std::string_view color = (i == path.size() - 1)
                        ? diagnostics::ansi::BOLD
                        : diagnostics::ansi::GREEN;
                    out << "  " << indent << connector
                        << colorize(display, color, colorEnabled) << "\n";
                }
            }
            out << "\n";
        }

        if (!foundPath)
        {
            out << "  No import paths found from entry points to "
                << targetDisplay << "\n";
        }
    }
}
