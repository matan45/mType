#pragma once
#include "DependencyGraph.hpp"
#include "../json/JsonValue.hpp"
#include <ostream>
#include <memory>
#include <string>
#include <unordered_set>

namespace project
{
    class DependencyGraphFormatter
    {
    public:
        // Colored terminal tree output
        static void renderTree(
            const DependencyGraph& graph,
            std::ostream& out,
            bool colorEnabled);

        // Graphviz DOT format
        static void renderDot(
            const DependencyGraph& graph,
            std::ostream& out);

        // JSON format using the project's JsonValue library
        static std::shared_ptr<json::JsonValue> toJson(
            const DependencyGraph& graph);

        // Cycle report to terminal
        static void renderCycles(
            const std::vector<std::vector<std::string>>& cycles,
            const std::string& projectRoot,
            std::ostream& out,
            bool colorEnabled);

        // "Why" path report
        static void renderWhy(
            const DependencyGraph& graph,
            const std::string& targetFile,
            std::ostream& out,
            bool colorEnabled);

    private:
        static std::string displayPath(
            const std::string& absolutePath,
            const std::string& projectRoot);

        static void renderTreeNode(
            const DependencyGraph& graph,
            const std::string& file,
            std::ostream& out,
            bool colorEnabled,
            const std::string& prefix,
            bool isLast,
            std::unordered_set<std::string>& visited);

        static std::string colorize(
            const std::string& text,
            std::string_view color,
            bool enabled);

        static std::string nodeColor(
            NodeKind kind,
            bool enabled);
    };
}
