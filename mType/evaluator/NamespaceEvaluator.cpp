#include "NamespaceEvaluator.hpp"
#include "Evaluator.hpp"
#include "../errors/UndefinedException.hpp"
#include "../errors/TypeException.hpp"
#include "../runtimeTypes/global/VariableDefinition.hpp"
#include <iostream>

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
        // Get the current namespace path and append this namespace to it
        auto currentPath = getCurrentNamespace();
        currentPath.push_back(node->getNamespaceName());
        
        enterNamespace(currentPath);
        
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
        // Add to global using directives (for immediate use)
        addUsingDirective(node->getNamespacePath());
        
        // Also store in current namespace definition (for persistent access)
        auto env = mainEvaluator->getEnvironment();
        auto nsManager = env->getNamespaceManager();
        if (nsManager) {
            auto currentNs = nsManager->getCurrentNamespace();
            if (currentNs) {
                currentNs->addUsingDirective(node->getNamespacePath());
            }
        }
        
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
        
        // First, try to find it as a namespace variable
        if (qualifiedName.size() >= 2) {
            std::vector<std::string> namespacePath(qualifiedName.begin(), qualifiedName.end() - 1);
            std::string varName = qualifiedName.back();
            
            auto variableManager = env->getVariableManager();
            if (variableManager) {
                auto varDef = variableManager->findVariableInNamespace(namespacePath, varName);
                
                // If not found with absolute path, try relative to current namespace
                if (!varDef) {
                    auto currentNamespacePath = env->getCurrentNamespacePath();
                    if (!currentNamespacePath.empty()) {
                        std::vector<std::string> relativeNamespacePath = currentNamespacePath;
                        relativeNamespacePath.insert(relativeNamespacePath.end(), namespacePath.begin(), namespacePath.end());
                        varDef = variableManager->findVariableInNamespace(relativeNamespacePath, varName);
                    }
                }
                
                if (varDef) {
                    return varDef->getValue();
                }
            }
        }
        
        // If not found as namespace variable, check if this is a static class field access
        // This handles both simple (ClassName::fieldName) and qualified (namespace::ClassName::fieldName)
        if (qualifiedName.size() >= 2) {
            std::string fieldName = qualifiedName.back();
            std::vector<std::string> classNameParts(qualifiedName.begin(), qualifiedName.end() - 1);
            
            // Join the class name parts with "::" to form qualified class name
            std::string className = "";
            for (size_t i = 0; i < classNameParts.size(); ++i) {
                if (i > 0) className += "::";
                className += classNameParts[i];
            }
            
            // Find the class definition
            auto classDef = env->findClass(className);
            if (classDef) {
                // Check if it's a static field
                auto field = classDef->getField(fieldName);
                if (field) {
                    if (field->isStatic()) {
                        auto value = field->getValue();
                        return value;
                    }
                }
            }
        }
        
        // Join qualified name with "::" for error message
        std::string fullName = "";
        for (size_t i = 0; i < qualifiedName.size(); ++i) {
            if (i > 0) fullName += "::";
            fullName += qualifiedName[i];
        }
        
        throw UndefinedException("Undefined variable: " + fullName, SourceLocation{});
    }

    void NamespaceEvaluator::assignQualifiedVariable(const std::vector<std::string>& qualifiedName, const Value& value)
    {
        auto env = mainEvaluator->getEnvironment();
        
        // First, try to find it as a namespace variable
        if (qualifiedName.size() >= 2) {
            std::vector<std::string> namespacePath(qualifiedName.begin(), qualifiedName.end() - 1);
            std::string varName = qualifiedName.back();
            
            auto variableManager = env->getVariableManager();
            if (variableManager) {
                auto varDef = variableManager->findVariableInNamespace(namespacePath, varName);
                if (varDef) {
                    if (varDef->isFinal()) {
                        std::string fullName = "";
                        for (size_t i = 0; i < qualifiedName.size(); ++i) {
                            if (i > 0) fullName += "::";
                            fullName += qualifiedName[i];
                        }
                        throw TypeException("Cannot reassign final variable: " + fullName, SourceLocation{});
                    }
                    varDef->setValue(value);
                    return;
                }
            }
        }
        
        // If not found as namespace variable, check if this is a static class field assignment (ClassName::fieldName)
        if (qualifiedName.size() == 2) {
            std::string className = qualifiedName[0];
            std::string fieldName = qualifiedName[1];
            
            // Find the class definition
            auto classDef = env->findClass(className);
            if (classDef) {
                // Check if it's a static field
                auto field = classDef->getField(fieldName);
                if (field && field->isStatic()) {
                    if (field->isFinal()) {
                        std::string fullName = className + "::" + fieldName;
                        throw TypeException("Cannot reassign final static field: " + fullName, SourceLocation{});
                    }
                    field->setValue(value);
                    return;
                }
            }
        }
        
        // Join qualified name with "::" for error message
        std::string fullName = "";
        for (size_t i = 0; i < qualifiedName.size(); ++i) {
            if (i > 0) fullName += "::";
            fullName += qualifiedName[i];
        }
        
        throw UndefinedException("Undefined variable: " + fullName, SourceLocation{});
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
