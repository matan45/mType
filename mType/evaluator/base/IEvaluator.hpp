#pragma once
#include "../../ast/ASTNode.hpp"
#include "../../value/ValueType.hpp"
#include <memory>
#include <string>
#include <vector>

namespace evaluator::base
{
    using namespace ast;
    using namespace value;
    
    /**
     * @brief Base interface for all evaluators following Interface Segregation Principle
     * @template T Return type of evaluation (typically Value)
     */
    template<typename T>
    class IEvaluator
    {
    public:
        virtual ~IEvaluator() = default;
        
        /**
         * @brief Evaluate an AST node and return result
         * @param node The AST node to evaluate
         * @return Evaluation result of type T
         */
        virtual T evaluate(ASTNode* node) = 0;
        
        /**
         * @brief Check if evaluator can handle given node type
         * @param node The AST node to check
         * @return true if this evaluator can handle the node
         */
        virtual bool canHandle(ASTNode* node) const = 0;
    };
    
    /**
     * @brief Specialized interface for expression evaluation
     */
    class IExpressionEvaluator : public IEvaluator<Value>
    {
    public:
        virtual ~IExpressionEvaluator() = default;
        
        // Type conversion utilities
        virtual bool isTruthy(const Value& value) const = 0;
        virtual std::string toString(const Value& value) const = 0;
        virtual float toFloat(const Value& value) const = 0;
        virtual int toInt(const Value& value) const = 0;
    };
    
    /**
     * @brief Specialized interface for statement evaluation
     */
    class IStatementEvaluator : public IEvaluator<Value>
    {
    public:
        virtual ~IStatementEvaluator() = default;
        
        // Control flow management
        virtual bool shouldBreakOrContinue() const = 0;
        virtual void handleBreak() = 0;
        virtual void handleContinue() = 0;
        virtual void resetLoopFlags() = 0;
    };
    
    /**
     * @brief Specialized interface for object evaluation
     */
    class IObjectEvaluator : public IEvaluator<Value>
    {
    public:
        virtual ~IObjectEvaluator() = default;
        
        // Method call operations
        virtual Value callMethod(std::shared_ptr<runtimeTypes::klass::ObjectInstance> object, 
                                const std::string& methodName,
                                const std::vector<Value>& args) = 0;
        
        // Static method operations
        virtual Value callStaticMethod(const std::string& className,
                                     const std::string& methodName,
                                     const std::vector<Value>& args) = 0;
        
        // Static member operations
        virtual Value accessStaticMember(const std::string& className,
                                       const std::string& memberName) = 0;
    };
}