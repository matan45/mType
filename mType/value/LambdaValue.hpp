#pragma once
#include "../evaluator/base/EvaluationContext.hpp"
#include "../value/CapturedVariable.hpp"
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

        // Captured 'this' instance for class member access
        std::shared_ptr<runtimeTypes::klass::ObjectInstance> capturedThisInstance;

        // Captured calling class for access control (especially for lambdas in static methods)
        std::string capturedCallingClass;

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

        // Memory safety and validation
        bool isValid() const;
        std::string getValidationStatus() const;
        void validateState() const; // Throws on invalid state

    private:
        // Memory optimization constants
        static constexpr size_t MAX_VALUE_CAPTURE_SIZE = 256; // bytes
        static constexpr size_t LARGE_OBJECT_THRESHOLD = 1024; // bytes

        // Helper methods
        void captureCurrentEnvironment();
        std::vector<std::string> analyzeVariableReferences() const;
        void validateArguments(const std::vector<Value>& arguments) const;
        void traverseForVariables(const ast::ASTNode* node, std::set<std::string>& variables) const;

        // Memory optimization helpers
        bool shouldCaptureByValue(const Value& value, ValueType type) const;
        size_t estimateValueSize(const Value& value, ValueType type) const;
        void addCapturedVariableOptimized(const std::string& name, const Value& value, ValueType type);
        void addCapturedVariableByReference(const std::string& name, const Value& value, ValueType type);
    };
}
