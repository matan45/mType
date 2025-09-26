#pragma once
#include "../evaluator/base/EvaluationContext.hpp"
#include "../value/ValueType.hpp"
#include <memory>
#include <vector>
#include <unordered_map>
#include <string>
#include <set>

// Forward declarations
namespace ast::nodes::expressions {
    class LambdaNode;
}

namespace runtimeTypes::klass {
    class ObjectInstance;
}

namespace value
{
    // Captured variable representation for closures
    struct CapturedVariable {
        std::string name;
        Value value;
        ValueType originalType;

        CapturedVariable(const std::string& n, const Value& v, ValueType t)
            : name(n), value(v), originalType(t) {}
    };

    /**
     * Enhanced lambda value with closure support and interface compatibility
     * Represents an unevaluated lambda expression that captures its environment
     */
    class LambdaValue
    {
    private:
        ast::nodes::expressions::LambdaNode* lambdaNode;
        std::shared_ptr<evaluator::base::EvaluationContext> capturedContext;

        // Closure storage - captured variable values at lambda creation time
        std::unordered_map<std::string, CapturedVariable> capturedVariables;

        // Interface implementation info
        std::string implementedInterface;
        std::string implementedMethod;
        bool isInterfaceImplementation;

    public:
        LambdaValue(ast::nodes::expressions::LambdaNode* node,
                    std::shared_ptr<evaluator::base::EvaluationContext> context);

        // Core accessors
        ast::nodes::expressions::LambdaNode* getLambda() const { return lambdaNode; }
        std::shared_ptr<evaluator::base::EvaluationContext> getContext() const { return capturedContext; }

        // Lambda invocation - main execution method
        Value invoke(const std::vector<Value>& arguments,
                    std::shared_ptr<evaluator::base::EvaluationContext> callContext);

        // Closure support
        const std::unordered_map<std::string, CapturedVariable>& getCapturedVariables() const { return capturedVariables; }
        void addCapturedVariable(const std::string& name, const Value& value, ValueType type);
        bool hasCapturedVariable(const std::string& name) const;
        const CapturedVariable* getCapturedVariable(const std::string& name) const;

        // Interface implementation
        void setInterfaceImplementation(const std::string& interface, const std::string& method);
        bool isImplementingInterface() const { return isInterfaceImplementation; }
        const std::string& getImplementedInterface() const { return implementedInterface; }
        const std::string& getImplementedMethod() const { return implementedMethod; }

        // Type information
        std::vector<ValueType> getParameterTypes() const;
        ValueType getReturnType() const;

        // Interface proxy creation for functional interfaces
        std::shared_ptr<runtimeTypes::klass::ObjectInstance> createInterfaceProxy() const;

    private:
        // Helper methods
        void captureCurrentEnvironment();
        std::vector<std::string> analyzeVariableReferences() const;
        void validateArguments(const std::vector<Value>& arguments) const;
        void traverseForVariables(const ast::ASTNode* node, std::set<std::string>& variables) const;
    };
}
