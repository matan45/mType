#pragma once
#include "base/IEvaluator.hpp"
#include "base/EvaluationContext.hpp"
#include "utils/ValueConverter.hpp"
#include "../token/TokenType.hpp"
#include "../ast/NodeClassesDeclaration.hpp"
#include "../ast/nodes/expressions/NullNode.hpp"
#include "../errors/SourceLocation.hpp"
#include <memory>

namespace evaluator
{
    using namespace base;
    using namespace utils;
    using namespace value;
    using namespace ast::nodes::expressions;
    using namespace ast::nodes::functions;
    using namespace ast::nodes::classes;
    using namespace ast::nodes::statements;
    using namespace token;
    
    /**
     * @brief Refactored Expression Evaluator following SOLID principles
     * - Single Responsibility: Only handles expression evaluation
     * - Open/Closed: Extensible through composition
     * - Liskov Substitution: Implements IExpressionEvaluator interface
     * - Interface Segregation: Uses specialized interfaces
     * - Dependency Inversion: Depends on abstractions (EvaluationContext)
     */
    class ExpressionEvaluator : public IExpressionEvaluator
    {
    private:
        std::shared_ptr<EvaluationContext> context;
        
        // Forward declarations for circular dependency resolution
        class IStatementEvaluator* stmtEvaluator;
        class IObjectEvaluator* objEvaluator;
        
    public:
        explicit ExpressionEvaluator(std::shared_ptr<EvaluationContext> ctx);
        ~ExpressionEvaluator() = default;
        
        // Main interface methods
        Value evaluate(ASTNode* node);
        bool canHandle(ASTNode* node) const;
        
        // Type conversion methods
        bool isTruthy(const Value& value) const;
        std::string toString(const Value& value) const;
        float toFloat(const Value& value) const;
        int toInt(const Value& value) const;
        
        // Expression evaluation methods (now private implementation details)
        Value evaluateIntegerNode(IntegerNode* node);
        Value evaluateFloatNode(FloatNode* node);
        Value evaluateStringNode(StringNode* node);
        Value evaluateBoolNode(BoolNode* node);
        Value evaluateNullNode(NullNode* node);
        Value evaluateVariableNode(VariableNode* node);
        Value evaluateBinaryExpNode(BinaryExpNode* node);
        Value evaluateTernaryExpNode(TernaryExpNode* node);
        Value evaluateUnaryExpNode(UnaryExpNode* node);
        Value evaluateFunctionCallNode(FunctionCallNode* node);
        Value evaluateMemberAccessNode(MemberAccessNode* node);
        Value evaluateMethodCallNode(MethodCallNode* node);
        Value evaluateNewNode(NewNode* node);
        Value evaluateAssignmentExpression(AssignmentNode* node);
        
        // Dependency injection for cross-evaluator communication
        void setStatementEvaluator(IStatementEvaluator* evaluator);
        void setObjectEvaluator(IObjectEvaluator* evaluator);
        
    private:
        // Helper methods for binary operations
        Value evaluateArithmetic(const Value& left, const Value& right, TokenType op);
        Value evaluateComparison(const Value& left, const Value& right, TokenType op);
        Value evaluateLogical(const Value& left, const Value& right, TokenType op);
        Value evaluateStringOperation(const Value& left, const Value& right, TokenType op);
        
        // Node type checking
        bool isExpressionNode(ASTNode* node) const;
        
        // Function return type validation
        void validateFunctionReturnType(ValueType expectedType, const Value& returnValue, 
                                       const std::string& functionName, 
                                       const SourceLocation& location);
    };
}
