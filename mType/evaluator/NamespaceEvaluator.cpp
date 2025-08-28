#include "NamespaceEvaluator.hpp"
#include "Evaluator.hpp"
#include "../errors/UndefinedException.hpp"
#include "../errors/TypeException.hpp"
#include "../runtimeTypes/global/VariableDefinition.hpp"

namespace evaluator
{
    using namespace errors;
    using namespace runtimeTypes::global;

    NamespaceEvaluator::NamespaceEvaluator(Evaluator* evaluator)
        : mainEvaluator(evaluator)
    {
    }

    Value NamespaceEvaluator::evaluateNamespaceNode(NamespaceNode* node)
    {
        std::vector<std::string> namespacePath = {node->getNamespaceName()};
        enterNamespace(namespacePath);
        
        Value lastValue = std::monostate{};
        
        try {
            for (const auto& statement : node->getDeclarations()) {
                lastValue = mainEvaluator->evaluate(statement.get());
            }
        }
        catch (...) {
            exitNamespace();
            throw;
        }
        
        exitNamespace();
        return lastValue;
    }

    Value NamespaceEvaluator::evaluateUsingNode(UsingNode* node)
    {
        addUsingDirective(node->getNamespacePath());
        return std::monostate{};
    }

    Value NamespaceEvaluator::evaluateQualifiedNameNode(QualifiedNameNode* node)
    {
        return accessQualifiedVariable(node->getQualifiers());
    }

    Value NamespaceEvaluator::evaluateQualifiedAssignmentNode(QualifiedAssignmentNode* node)
    {
        Value newValue = mainEvaluator->evaluate(node->getValue());
        assignQualifiedVariable(node->getQualifiedName(), newValue);
        return newValue;
    }

    std::vector<std::string> NamespaceEvaluator::resolveQualifiedName(const std::vector<std::string>& parts)
    {
        // Simple implementation - return as is
        // TODO: Implement proper namespace resolution
        return parts;
    }

    Value NamespaceEvaluator::accessQualifiedVariable(const std::vector<std::string>& qualifiedName)
    {
        auto env = mainEvaluator->getEnvironment();
        
        // Join qualified name with "::"
        std::string fullName = "";
        for (size_t i = 0; i < qualifiedName.size(); ++i) {
            if (i > 0) fullName += "::";
            fullName += qualifiedName[i];
        }
        
        auto varDef = env->findVariable(fullName);
        if (!varDef) {
            throw UndefinedException("Undefined variable: " + fullName, SourceLocation{});
        }
        
        return varDef->getValue();
    }

    void NamespaceEvaluator::assignQualifiedVariable(const std::vector<std::string>& qualifiedName, const Value& value)
    {
        auto env = mainEvaluator->getEnvironment();
        
        // Join qualified name with "::"
        std::string fullName = "";
        for (size_t i = 0; i < qualifiedName.size(); ++i) {
            if (i > 0) fullName += "::";
            fullName += qualifiedName[i];
        }
        
        auto varDef = env->findVariable(fullName);
        if (!varDef) {
            throw UndefinedException("Undefined variable: " + fullName, SourceLocation{});
        }
        
        if (varDef->isFinal()) {
            throw TypeException("Cannot reassign final variable: " + fullName, SourceLocation{});
        }
        
        varDef->setValue(value);
    }

    Value NamespaceEvaluator::callQualifiedFunction(const std::vector<std::string>& qualifiedName,
                                                    const std::vector<Value>& args)
    {
        // TODO: Implement qualified function calls
        return std::monostate{};
    }

    void NamespaceEvaluator::enterNamespace(const std::vector<std::string>& namespacePath)
    {
        auto env = mainEvaluator->getEnvironment();
        env->enterNamespace(namespacePath);
    }

    void NamespaceEvaluator::exitNamespace()
    {
        auto env = mainEvaluator->getEnvironment();
        env->exitNamespace();
    }

    std::vector<std::string> NamespaceEvaluator::getCurrentNamespace() const
    {
        auto env = mainEvaluator->getEnvironment();
        return env->getCurrentNamespacePath();
    }

    void NamespaceEvaluator::addUsingDirective(const std::vector<std::string>& namespacePath)
    {
        auto env = mainEvaluator->getEnvironment();
        env->addUsingDirective(namespacePath);
    }

    std::vector<std::vector<std::string>> NamespaceEvaluator::getActiveUsingDirectives() const
    {
        auto env = mainEvaluator->getEnvironment();
        return env->getUsingDirectives();
    }

    bool NamespaceEvaluator::isNamespaceExists(const std::vector<std::string>& namespacePath) const
    {
        // TODO: Implement namespace existence check
        return false;
    }

    bool NamespaceEvaluator::isSymbolInNamespace(const std::vector<std::string>& namespacePath,
                                                 const std::string& symbolName) const
    {
        // TODO: Implement symbol existence check in namespace
        return false;
    }
}
