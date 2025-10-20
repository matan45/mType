#pragma once
#include "base/EvaluationContext.hpp"
#include "utils/ValueConverter.hpp"
#include "utils/NodeDispatcher.hpp"
#include "interfaces/IExpressionEvaluator.hpp"
#include "interfaces/IValueConverter.hpp"
#include "../token/TokenType.hpp"
#include "../ast/NodeClassesDeclaration.hpp"
#include "../ast/nodes/expressions/NullNode.hpp"
#include "../ast/nodes/expressions/LambdaInterfaceInvocationNode.hpp"
#include "../ast/nodes/expressions/AwaitExpression.hpp"
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
    class SuperCallHandler;
    class CastAndTypeCheckHandler;
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
     * - Liskov Substitution: Implements IExpressionEvaluator and IValueConverter interfaces
     * - Interface Segregation: Implements two focused interfaces
     * - Dependency Inversion: Depends on abstractions (EvaluationContext)
     *
     * Implements both IExpressionEvaluator (core evaluation) and IValueConverter
     * (type conversions) to provide complete expression evaluation functionality.
     * Clients can depend on either interface based on their needs.
     */
    // Forward declarations
    namespace expressions {
        class LiteralEvaluator;
        class BinaryOperationEvaluator;
    }

    class ExpressionEvaluator : public interfaces::IExpressionEvaluator,
                                 public interfaces::IValueConverter
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
        std::unique_ptr<expressions::SuperCallHandler> superCallHandler;
        std::unique_ptr<expressions::CastAndTypeCheckHandler> castAndTypeCheckHandler;

        // Node dispatcher for O(1) dispatch instead of cascading dynamic_cast
        utils::NodeDispatcher<ExpressionEvaluator> dispatcher;

        // Forward declarations for circular dependency resolution
        class StatementEvaluator* stmtEvaluator;
        class ObjectEvaluator* objEvaluator;
        class EvaluatorCoordinator* coordinator;  // For delegating nodes we can't handle (like AwaitExpression)

    public:
        explicit ExpressionEvaluator(std::shared_ptr<EvaluationContext> ctx);
        ~ExpressionEvaluator() override;

        // IExpressionEvaluator interface implementation
        Value evaluate(ASTNode* node) override;
        bool canHandle(ASTNode* node) const override;

        // IValueConverter interface implementation
        bool isTruthy(const Value& value) const override;
        std::string toString(const Value& value) const override;
        float toFloat(const Value& value) const override;
        int toInt(const Value& value) const override;

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

        // NEW: Cast and type checking expressions
        Value evaluateCastExpression(CastExpression* node);
        Value evaluateInstanceOfExpression(InstanceOfExpression* node);

        // NOTE: AwaitExpression is handled by EvaluatorCoordinator for async/await support

        // Dependency injection for cross-evaluator communication
        void setStatementEvaluator(StatementEvaluator* evaluator);
        void setObjectEvaluator(ObjectEvaluator* evaluator);
        void setCoordinator(EvaluatorCoordinator* coord);

    private:
        // Initialize dispatcher with all handler registrations
        void initializeDispatcher();

        // Helper methods for binary operations
        Value evaluateArithmetic(const Value& left, const Value& right, TokenType op, const errors::SourceLocation& location);
        Value evaluateComparison(const Value& left, const Value& right, TokenType op, const errors::SourceLocation& location);

        // Node type checking - now delegated to NodeTypeRegistry
        bool isExpressionNode(ASTNode* node) const;

        // Multi-dimensional array access helpers
        std::optional<Value> extractMultiDimensionalAccess(IndexAccessNode* node, std::vector<size_t>& indices);
        Value evaluateDirectMultiDimensionalAccess(const Value& baseArray, const std::vector<size_t>& indices, const SourceLocation& location);

        // Helper for default values
        Value getDefaultValueForType(const ::parser::TypeInfo& elementType);
    };
}
