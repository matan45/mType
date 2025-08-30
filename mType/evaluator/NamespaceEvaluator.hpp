#pragma once
#include "../value/ValueType.hpp"
#include "../ast/nodes/namespaces/NamespaceNode.hpp"
#include "../ast/nodes/namespaces/UsingNode.hpp"
#include "../ast/nodes/namespaces/QualifiedNameNode.hpp"
#include "../ast/nodes/statements/QualifiedAssignmentNode.hpp"
#include <memory>
#include <vector>
#include <string>

namespace evaluator
{
    using namespace value;
    using namespace ast::nodes::namespaces;
    using namespace ast::nodes::statements;
    
    class Evaluator;
    
    class NamespaceEvaluator
    {
    private:
        Evaluator* mainEvaluator;
        
    public:
        explicit NamespaceEvaluator(Evaluator* evaluator);
        ~NamespaceEvaluator() = default;
        
        // Namespace evaluation methods
        Value evaluateNamespaceNode(NamespaceNode* node);
        Value evaluateUsingNode(UsingNode* node);
        Value evaluateQualifiedNameNode(QualifiedNameNode* node);
        Value evaluateQualifiedAssignmentNode(QualifiedAssignmentNode* node);
        
        // Helper methods
        std::vector<std::string> resolveQualifiedName(const std::vector<std::string>& parts);
        Value accessQualifiedVariable(const std::vector<std::string>& qualifiedName);
        void assignQualifiedVariable(const std::vector<std::string>& qualifiedName, const Value& value);
        Value callQualifiedFunction(const std::vector<std::string>& qualifiedName, 
                                   const std::vector<Value>& args);
        
        // Namespace management
        void enterNamespace(const std::vector<std::string>& namespacePath);
        void exitNamespace();
        std::vector<std::string> getCurrentNamespace() const;
        
        // Using directive management
        void addUsingDirective(const std::vector<std::string>& namespacePath);
        std::vector<std::vector<std::string>> getActiveUsingDirectives() const;
        
        // Symbol resolution
        bool isNamespaceExists(const std::vector<std::string>& namespacePath) const;
        bool isSymbolInNamespace(const std::vector<std::string>& namespacePath, 
                                 const std::string& symbolName) const;
    };
}
