#pragma once
#include "../../../ast/ASTNode.hpp"
#include "../../../environment/Environment.hpp"
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <string>

namespace vm::compiler::validation
{
    /**
     * Validates field initialization dependencies to detect circular references
     * Analyzes which classes are instantiated in field initializers to build a dependency graph
     */
    class FieldInitializationValidator
    {
    public:
        explicit FieldInitializationValidator(std::shared_ptr<environment::Environment> env);

        /**
         * Validate all classes for circular field initialization dependencies
         * Throws TypeException if circular dependency is detected
         */
        void validateFieldInitializations(ast::ASTNode* root);

    private:
        std::shared_ptr<environment::Environment> environment;

        // Dependency graph: className -> set of classes it depends on
        std::unordered_map<std::string, std::unordered_set<std::string>> staticFieldDependencies;
        std::unordered_map<std::string, std::unordered_set<std::string>> instanceFieldDependencies;

        // Analyze a single class's field initializers
        void analyzeClassFields(ast::nodes::classes::ClassNode* node);

        // Extract class dependencies from an expression (finds all 'new ClassName()' calls)
        void extractDependencies(ast::ASTNode* expr, std::unordered_set<std::string>& dependencies);

        // Detect cycles in the dependency graph
        bool hasCycle(
            const std::unordered_map<std::string, std::unordered_set<std::string>>& graph,
            const std::string& startClass,
            std::vector<std::string>& cycle
        );

        // DFS helper for cycle detection
        bool hasCycleDFS(
            const std::unordered_map<std::string, std::unordered_set<std::string>>& graph,
            const std::string& currentClass,
            std::unordered_set<std::string>& visited,
            std::unordered_set<std::string>& recursionStack,
            std::vector<std::string>& path
        );

        // Traverse AST to find all classes
        void findAllClasses(ast::ASTNode* node, std::vector<ast::nodes::classes::ClassNode*>& classes);
    };
}
