#include "FieldInitializationValidator.hpp"
#include <cstddef>
#include "../../../ast/nodes/statements/ProgramNode.hpp"
#include "../../../ast/nodes/statements/BlockNode.hpp"
#include "../../../ast/nodes/statements/ImportNode.hpp"
#include "../../../ast/nodes/classes/ClassNode.hpp"
#include "../../../ast/nodes/classes/FieldNode.hpp"
#include "../../../ast/nodes/classes/NewNode.hpp"
#include "../../../ast/nodes/expressions/BinaryExpNode.hpp"
#include "../../../ast/nodes/expressions/UnaryExpNode.hpp"
#include "../../../ast/nodes/expressions/TernaryExpNode.hpp"
#include "../../../errors/TypeException.hpp"
#include <sstream>

namespace vm::compiler::validation
{
    FieldInitializationValidator::FieldInitializationValidator(std::shared_ptr<environment::Environment> env)
        : environment(env)
    {
    }

    void FieldInitializationValidator::validateFieldInitializations(ast::ASTNode* root)
    {
        if (!root) return;

        // Find all classes in the AST
        std::vector<ast::nodes::classes::ClassNode*> classes;
        findAllClasses(root, classes);

        // Analyze each class's field initializers to build dependency graph
        for (ast::nodes::classes::ClassNode* classNode : classes)
        {
            analyzeClassFields(classNode);
        }

        // Check for cycles in static field dependencies
        for (const auto& [className, _] : staticFieldDependencies)
        {
            std::vector<std::string> cycle;
            if (hasCycle(staticFieldDependencies, className, cycle))
            {
                std::ostringstream oss;
                oss << "Circular static field initialization detected: ";
                for (size_t i = 0; i < cycle.size(); ++i)
                {
                    if (i > 0) oss << " -> ";
                    oss << cycle[i];
                }
                throw errors::TypeException(oss.str(), errors::SourceLocation());
            }
        }

        // Check for cycles in instance field dependencies
        for (const auto& [className, _] : instanceFieldDependencies)
        {
            std::vector<std::string> cycle;
            if (hasCycle(instanceFieldDependencies, className, cycle))
            {
                std::ostringstream oss;
                oss << "Circular instance field initialization detected: ";
                for (size_t i = 0; i < cycle.size(); ++i)
                {
                    if (i > 0) oss << " -> ";
                    oss << cycle[i];
                }
                throw errors::TypeException(oss.str(), errors::SourceLocation());
            }
        }
    }

    void FieldInitializationValidator::analyzeClassFields(ast::nodes::classes::ClassNode* node)
    {
        std::string className = node->getClassName();

        for (const auto& fieldPtr : node->getFields())
        {
            if (auto* fieldNode = dynamic_cast<ast::nodes::classes::FieldNode*>(fieldPtr.get()))
            {
                if (fieldNode->hasInitialValue())
                {
                    std::unordered_set<std::string> dependencies;
                    extractDependencies(fieldNode->getInitialValue(), dependencies);

                    if (fieldNode->getIsStatic())
                    {
                        staticFieldDependencies[className].insert(dependencies.begin(), dependencies.end());
                    }
                    else
                    {
                        instanceFieldDependencies[className].insert(dependencies.begin(), dependencies.end());
                    }
                }
            }
        }
    }

    void FieldInitializationValidator::extractDependencies(
        ast::ASTNode* expr,
        std::unordered_set<std::string>& dependencies)
    {
        if (!expr) return;

        // Check if this is a NewNode (new ClassName())
        if (auto* newObj = dynamic_cast<ast::nodes::classes::NewNode*>(expr))
        {
            dependencies.insert(newObj->getClassName());
            return;  // Found a dependency, no need to recurse into arguments
        }

        // Recursively search child nodes
        // This is a simplified traversal - in a full implementation, you'd visit all node types
        // For now, we'll just handle the most common cases

        // Handle common expression types that might contain NewNode
        if (auto* binary = dynamic_cast<ast::nodes::expressions::BinaryExpNode*>(expr))
        {
            extractDependencies(binary->getLeft(), dependencies);
            extractDependencies(binary->getRight(), dependencies);
        }
        else if (auto* unary = dynamic_cast<ast::nodes::expressions::UnaryExpNode*>(expr))
        {
            extractDependencies(unary->getOperand(), dependencies);
        }
        else if (auto* ternary = dynamic_cast<ast::nodes::expressions::TernaryExpNode*>(expr))
        {
            extractDependencies(ternary->getCondition(), dependencies);
            extractDependencies(ternary->getTrueExpression(), dependencies);
            extractDependencies(ternary->getFalseExpression(), dependencies);
        }
        // Add more expression types as needed
    }

    bool FieldInitializationValidator::hasCycle(
        const std::unordered_map<std::string, std::unordered_set<std::string>>& graph,
        const std::string& startClass,
        std::vector<std::string>& cycle)
    {
        std::unordered_set<std::string> visited;
        std::unordered_set<std::string> recursionStack;
        std::vector<std::string> path;

        if (hasCycleDFS(graph, startClass, visited, recursionStack, path))
        {
            cycle = path;
            return true;
        }
        return false;
    }

    bool FieldInitializationValidator::hasCycleDFS(
        const std::unordered_map<std::string, std::unordered_set<std::string>>& graph,
        const std::string& currentClass,
        std::unordered_set<std::string>& visited,
        std::unordered_set<std::string>& recursionStack,
        std::vector<std::string>& path)
    {
        // If already in recursion stack, we found a cycle
        if (recursionStack.count(currentClass))
        {
            // Add current class to complete the cycle
            path.push_back(currentClass);
            return true;
        }

        // If already visited (but not in recursion stack), no cycle from this path
        if (visited.count(currentClass))
        {
            return false;
        }

        // Mark as visited and add to recursion stack
        visited.insert(currentClass);
        recursionStack.insert(currentClass);
        path.push_back(currentClass);

        // Check all dependencies
        auto it = graph.find(currentClass);
        if (it != graph.end())
        {
            for (const std::string& dependency : it->second)
            {
                if (hasCycleDFS(graph, dependency, visited, recursionStack, path))
                {
                    return true;
                }
            }
        }

        // Remove from recursion stack and path (backtrack)
        recursionStack.erase(currentClass);
        path.pop_back();
        return false;
    }

    void FieldInitializationValidator::findAllClasses(ast::ASTNode* node, std::vector<ast::nodes::classes::ClassNode*>& classes)
    {
        if (!node) return;

        if (auto* classNode = dynamic_cast<ast::nodes::classes::ClassNode*>(node))
        {
            classes.push_back(classNode);
            return;
        }

        if (auto* programNode = dynamic_cast<ast::ProgramNode*>(node))
        {
            for (const auto& stmt : programNode->getStatements())
            {
                findAllClasses(stmt.get(), classes);
            }
        }
        else if (auto* blockNode = dynamic_cast<ast::BlockNode*>(node))
        {
            for (const auto& stmt : blockNode->getStatements())
            {
                findAllClasses(stmt.get(), classes);
            }
        }
        else if (auto* importNode = dynamic_cast<ast::nodes::statements::ImportNode*>(node))
        {
            if (importNode->isResolved() && importNode->getImportedAST())
            {
                findAllClasses(importNode->getImportedAST(), classes);
            }
        }
    }
}
