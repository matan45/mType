#pragma once
#include "base/EvaluationContext.hpp"
#include "utils/ValueConverter.hpp"
#include "utils/NodeDispatcher.hpp"
#include "../token/TokenType.hpp"
#include "../ast/NodeClassesDeclaration.hpp"
#include "../ast/nodes/expressions/NullNode.hpp"
#include "../ast/nodes/expressions/LambdaInterfaceInvocationNode.hpp"
#include "../errors/SourceLocation.hpp"
#include "../parser/TypeParser.hpp"
#include <memory>
#include <optional>

// Forward declarations for specialized handlers
namespace evaluator {
namespace expressions {
    class LiteralEvaluator;
    class BinaryOperationEvaluator;
    class CallHandler;
    class ArrayHandler;
    class UnaryOperationHandler;
    class AccessHandler;
}
}

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
    // Forward declarations
    namespace expressions {
        class LiteralEvaluator;
        class BinaryOperationEvaluator;
    }

    class ExpressionEvaluator
    {
    private:
        std::shared_ptr<EvaluationContext> context;

        // Specialized expression handlers
        std::unique_ptr<expressions::LiteralEvaluator> literalEvaluator;
        std::unique_ptr<expressions::BinaryOperationEvaluator> binaryOpEvaluator;
        std::unique_ptr<expressions::CallHandler> callHandler;
        std::unique_ptr<expressions::ArrayHandler> arrayHandler;
        std::unique_ptr<expressions::UnaryOperationHandler> unaryOpHandler;
        std::unique_ptr<expressions::AccessHandler> accessHandler;

        // Node dispatcher for O(1) dispatch instead of cascading dynamic_cast
        utils::NodeDispatcher<ExpressionEvaluator> dispatcher;

        // Forward declarations for circular dependency resolution
        class StatementEvaluator* stmtEvaluator;
        class ObjectEvaluator* objEvaluator;

    public:
        explicit ExpressionEvaluator(std::shared_ptr<EvaluationContext> ctx);
        ~ExpressionEvaluator();

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
        Value evaluateArrayCreationNode(ArrayCreationNode* node);
        Value evaluateArrayLiteralNode(ArrayLiteralNode* node);
        Value evaluateIndexAccessNode(IndexAccessNode* node);
        Value evaluateLambdaNode(LambdaNode* node);
        Value evaluateLambdaInterfaceInvocationNode(LambdaInterfaceInvocationNode* node);

        // NEW: Super expressions for inheritance
        Value evaluateSuperConstructorCallNode(SuperConstructorCallNode* node);
        Value evaluateSuperMethodCallNode(SuperMethodCallNode* node);

        // Dependency injection for cross-evaluator communication
        void setStatementEvaluator(StatementEvaluator* evaluator);
        void setObjectEvaluator(ObjectEvaluator* evaluator);

    private:
        // Initialize dispatcher with all handler registrations
        void initializeDispatcher();

        // Helper methods for binary operations
        Value evaluateArithmetic(const Value& left, const Value& right, TokenType op);
        Value evaluateComparison(const Value& left, const Value& right, TokenType op);
        Value evaluateLogical(const Value& left, const Value& right, TokenType op);
        Value evaluateStringOperation(const Value& left, const Value& right, TokenType op);

        // Node type checking - now delegated to NodeTypeRegistry
        bool isExpressionNode(ASTNode* node) const;

        // Multi-dimensional array access helpers
        std::optional<Value> extractMultiDimensionalAccess(IndexAccessNode* node, std::vector<size_t>& indices);
        Value evaluateDirectMultiDimensionalAccess(const Value& baseArray, const std::vector<size_t>& indices, const SourceLocation& location);

        // Helper for default values
        Value getDefaultValueForType(const ::parser::TypeInfo& elementType);
    };
}
