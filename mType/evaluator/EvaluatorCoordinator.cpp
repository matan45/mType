#include "EvaluatorCoordinator.hpp"
#include "../runtimeTypes/klass/ObjectInstance.hpp"
#include "../value/PromiseValue.hpp"
#include "../value/AsyncPromiseValue.hpp"
#include "../ast/nodes/expressions/AwaitExpression.hpp"
#include "../exception/SuspendException.hpp"

namespace evaluator
{
    using namespace runtimeTypes::klass;

    
    EvaluatorCoordinator::EvaluatorCoordinator(std::shared_ptr<Environment> environment)
        : context(std::make_shared<EvaluationContext>(environment))
    {
        // Create specialized evaluators with shared context
        exprEvaluator = std::make_unique<ExpressionEvaluator>(context);
        stmtEvaluator = std::make_unique<StatementEvaluator>(context);
        objEvaluator = std::make_unique<ObjectEvaluator>(context);
        
        // Setup cross-evaluator dependencies
        setupEvaluatorDependencies();
    }
    
    void EvaluatorCoordinator::setupEvaluatorDependencies()
    {
        // Expression evaluator dependencies
        exprEvaluator->setStatementEvaluator(stmtEvaluator.get());
        exprEvaluator->setObjectEvaluator(objEvaluator.get());
        exprEvaluator->setCoordinator(this);  // For delegating nodes like AwaitExpression

        // Statement evaluator dependencies
        stmtEvaluator->setExpressionEvaluator(exprEvaluator.get());
        stmtEvaluator->setObjectEvaluator(objEvaluator.get());

        // Object evaluator dependencies
        objEvaluator->setExpressionEvaluator(exprEvaluator.get());
        objEvaluator->setStatementEvaluator(stmtEvaluator.get());
    }
    
    Value EvaluatorCoordinator::evaluate(ASTNode* node)
    {
        if (!node) {
            return std::monostate{};
        }

        return routeEvaluation(node);
    }
    
    Value EvaluatorCoordinator::routeEvaluation(ASTNode* node)
    {
        // Special case: AwaitExpression must be handled by coordinator for async/await support
        if (dynamic_cast<AwaitExpression*>(node)) {
            return node->accept(*this);
        }

        // Route to appropriate specialized evaluator based on node type
        // Priority order: Statements first (to handle declarations properly),
        // then Objects (to handle class operations), then Expressions
        if (stmtEvaluator->canHandle(node)) {
            return stmtEvaluator->evaluate(node);
        }
        else if (objEvaluator->canHandle(node)) {
            return objEvaluator->evaluate(node);
        }
        else if (exprEvaluator->canHandle(node)) {
            return exprEvaluator->evaluate(node);
        }
        else {
            // Fallback to visitor pattern for unhandled nodes
            return node->accept(*this);
        }
    }
    
    // ASTVisitor interface implementations (delegate to appropriate evaluator)
    Value EvaluatorCoordinator::visitProgramNode(ProgramNode* node)
    {
        return stmtEvaluator->evaluateProgramNode(node);
    }
    
    Value EvaluatorCoordinator::visitBlockNode(BlockNode* node)
    {
        return stmtEvaluator->evaluateBlockNode(node);
    }
    
    Value EvaluatorCoordinator::visitFloatNode(FloatNode* node)
    {
        return exprEvaluator->evaluateFloatNode(node);
    }
    
    Value EvaluatorCoordinator::visitIntegerNode(IntegerNode* node)
    {
        return exprEvaluator->evaluateIntegerNode(node);
    }
    
    Value EvaluatorCoordinator::visitStringNode(StringNode* node)
    {
        return exprEvaluator->evaluateStringNode(node);
    }
    
    Value EvaluatorCoordinator::visitBoolNode(BoolNode* node)
    {
        return exprEvaluator->evaluateBoolNode(node);
    }
    
    Value EvaluatorCoordinator::visitVariableNode(VariableNode* node)
    {
        return exprEvaluator->evaluateVariableNode(node);
    }

    Value EvaluatorCoordinator::visitAssignmentNode(AssignmentNode* node)
    {
        return stmtEvaluator->evaluateAssignmentNode(node);
    }
    
    Value EvaluatorCoordinator::visitBinaryOpNode(BinaryOpNode* node)
    {
        return exprEvaluator->evaluateBinaryExpNode(node);
    }
    
    Value EvaluatorCoordinator::visitTernaryOpNode(TernaryOpNode* node)
    {
        return exprEvaluator->evaluateTernaryExpNode(node);
    }
    
    Value EvaluatorCoordinator::visitUnaryOpNode(UnaryOpNode* node)
    {
        return exprEvaluator->evaluateUnaryExpNode(node);
    }
    
    Value EvaluatorCoordinator::visitIfNode(IfNode* node)
    {
        return stmtEvaluator->evaluateIfNode(node);
    }
    
    Value EvaluatorCoordinator::visitWhileNode(WhileNode* node)
    {
        return stmtEvaluator->evaluateWhileNode(node);
    }
    
    Value EvaluatorCoordinator::visitDoWhileNode(DoWhileNode* node)
    {
        return stmtEvaluator->evaluateDoWhileNode(node);
    }
    
    Value EvaluatorCoordinator::visitForNode(ForNode* node)
    {
        return stmtEvaluator->evaluateForNode(node);
    }
    
    Value EvaluatorCoordinator::visitBreakNode(BreakNode* node)
    {
        return stmtEvaluator->evaluateBreakNode(node);
    }
    
    Value EvaluatorCoordinator::visitContinueNode(ContinueNode* node)
    {
        return stmtEvaluator->evaluateContinueNode(node);
    }
    
    Value EvaluatorCoordinator::visitFunctionNode(FunctionNode* node)
    {
        return stmtEvaluator->evaluateFunctionNode(node);
    }
    
    Value EvaluatorCoordinator::visitFunctionCallNode(FunctionCallNode* node)
    {
        return exprEvaluator->evaluateFunctionCallNode(node);
    }
    
    Value EvaluatorCoordinator::visitReturnNode(ReturnNode* node)
    {
        return stmtEvaluator->evaluateReturnNode(node);
    }
    
    Value EvaluatorCoordinator::visitSwitchNode(SwitchNode* node)
    {
        return stmtEvaluator->evaluateSwitchNode(node);
    }
    
    Value EvaluatorCoordinator::visitCaseNode(CaseNode* node)
    {
        return stmtEvaluator->evaluateCaseNode(node);
    }
    
    Value EvaluatorCoordinator::visitImportNode(ImportNode* node)
    {
        return stmtEvaluator->evaluateImportNode(node);
    }
    
    Value EvaluatorCoordinator::visitDefaultCaseNode(DefaultCaseNode* node)
    {
        return stmtEvaluator->evaluateDefaultCaseNode(node);
    }

    Value EvaluatorCoordinator::visitNullNode(NullNode* node)
    {
        return exprEvaluator->evaluateNullNode(node);
    }
    
    Value EvaluatorCoordinator::visitMemberAssignmentNode(MemberAssignmentNode* node)
    {
        return objEvaluator->evaluateMemberAssignmentNode(node);
    }

    Value EvaluatorCoordinator::visitIndexAssignmentNode(IndexAssignmentNode* node)
    {
        return objEvaluator->evaluateIndexAssignmentNode(node);
    }
    
    Value EvaluatorCoordinator::visitMethodCallNode(MethodCallNode* node)
    {
        return objEvaluator->evaluateMethodCallNode(node);
    }
    
    Value EvaluatorCoordinator::visitMemberAccessNode(MemberAccessNode* node)
    {
        return objEvaluator->evaluateMemberAccessNode(node);
    }
    
    Value EvaluatorCoordinator::visitNewNode(NewNode* node)
    {
        return objEvaluator->evaluateNewNode(node);
    }
    
    Value EvaluatorCoordinator::visitMethodNode(MethodNode* node)
    {
        return objEvaluator->evaluateMethodNode(node);
    }
    
    Value EvaluatorCoordinator::visitConstructorNode(ConstructorNode* node)
    {
        return objEvaluator->evaluateConstructorNode(node);
    }
    
    Value EvaluatorCoordinator::visitFieldNode(FieldNode* node)
    {
        return objEvaluator->evaluateFieldNode(node);
    }
    
    Value EvaluatorCoordinator::visitClassNode(ClassNode* node)
    {
        return objEvaluator->evaluateClassNode(node);
    }

    Value EvaluatorCoordinator::visitInterfaceNode(InterfaceNode* node)
    {
        return objEvaluator->evaluateInterfaceNode(node);
    }

    // Helper methods (delegate to appropriate evaluator)
    bool EvaluatorCoordinator::isTruthy(const Value& value) const
    {
        return exprEvaluator->isTruthy(value);
    }
    
    std::string EvaluatorCoordinator::toString(const Value& value) const
    {
        return exprEvaluator->toString(value);
    }
    
    float EvaluatorCoordinator::toFloat(const Value& value) const
    {
        return exprEvaluator->toFloat(value);
    }
    
    int EvaluatorCoordinator::toInt(const Value& value) const
    {
        return exprEvaluator->toInt(value);
    }
    
    // State management (delegate to context)
    bool EvaluatorCoordinator::shouldReturn() const
    {
        return context->shouldReturn();
    }
    
    void EvaluatorCoordinator::setReturned(bool returned)
    {
        context->setReturned(returned);
    }
    
    Value EvaluatorCoordinator::getReturnValue()
    {
        return context->getReturnValue();
    }
    
    void EvaluatorCoordinator::pushReturnValue(const Value& value)
    {
        context->pushReturnValue(value);
    }
    
    std::shared_ptr<ObjectInstance> EvaluatorCoordinator::getCurrentInstance() const
    {
        return context->getCurrentInstance();
    }

    // Collection visitor methods
    Value EvaluatorCoordinator::visitArrayCreationNode(ArrayCreationNode* node)
    {
        return exprEvaluator->evaluateArrayCreationNode(node);
    }

    Value EvaluatorCoordinator::visitArrayLiteralNode(ArrayLiteralNode* node)
    {
        return exprEvaluator->evaluateArrayLiteralNode(node);
    }

    Value EvaluatorCoordinator::visitIndexAccessNode(IndexAccessNode* node)
    {
        return exprEvaluator->evaluateIndexAccessNode(node);
    }

    Value EvaluatorCoordinator::visitForEachNode(ForEachNode* node)
    {
        return stmtEvaluator->evaluateForEachNode(node);
    }

    Value EvaluatorCoordinator::visitLambdaNode(LambdaNode* node)
    {
        return exprEvaluator->evaluateLambdaNode(node);
    }

    Value EvaluatorCoordinator::visitSuperConstructorCallNode(SuperConstructorCallNode* node)
    {
        return exprEvaluator->evaluateSuperConstructorCallNode(node);
    }

    Value EvaluatorCoordinator::visitSuperMethodCallNode(SuperMethodCallNode* node)
    {
        return exprEvaluator->evaluateSuperMethodCallNode(node);
    }

    Value EvaluatorCoordinator::visitSuperMemberAccessNode(SuperMemberAccessNode* node)
    {
        return exprEvaluator->evaluateSuperMemberAccessNode(node);
    }

    Value EvaluatorCoordinator::visitSuperMemberAssignmentNode(SuperMemberAssignmentNode* node)
    {
        return exprEvaluator->evaluateSuperMemberAssignmentNode(node);
    }

    Value EvaluatorCoordinator::visitCastExpression(CastExpression* node)
    {
        return exprEvaluator->evaluateCastExpression(node);
    }

    Value EvaluatorCoordinator::visitInstanceOfExpression(InstanceOfExpression* node)
    {
        return exprEvaluator->evaluateInstanceOfExpression(node);
    }

    Value EvaluatorCoordinator::visitAwaitExpression(AwaitExpression* node)
    {
        // Evaluate the expression being awaited
        Value awaitedValue = evaluate(node->getExpressionPtr());

        // Check if the value is a Promise
        if (!std::holds_alternative<std::shared_ptr<PromiseValue>>(awaitedValue))
        {
            throw std::runtime_error("await can only be used on Promise values");
        }

        // Get the promise
        auto promise = std::get<std::shared_ptr<PromiseValue>>(awaitedValue);

        // Defensive null check
        if (!promise)
        {
            throw std::runtime_error("Null promise in await expression");
        }

        // FAST PATH: Promise already fulfilled, return immediately
        if (promise->isFulfilled())
        {
            return promise->getValue();
        }

        // SLOW PATH: Promise not yet fulfilled - suspend current task
        // Try to cast to AsyncPromiseValue to support continuations
        auto asyncPromise = std::dynamic_pointer_cast<value::AsyncPromiseValue>(promise);

        if (!asyncPromise)
        {
            // Promise doesn't support async continuations, use efficient blocking wait
            // Uses condition variable for zero CPU usage instead of busy-wait polling
            const int MAX_WAIT_MS = 10000;
            return promise->waitForValue(MAX_WAIT_MS);
        }

        // Throw SuspendException to unwind the call stack
        // The EventLoop will catch this and register a continuation
        throw exception::SuspendException(asyncPromise);
    }

    Value EvaluatorCoordinator::visitTryNode(TryNode* node)
    {
        return stmtEvaluator->evaluateTryNode(node);
    }

    Value EvaluatorCoordinator::visitCatchNode(CatchNode* node)
    {
        return stmtEvaluator->evaluateCatchNode(node);
    }

    Value EvaluatorCoordinator::visitThrowNode(ThrowNode* node)
    {
        return stmtEvaluator->evaluateThrowNode(node);
    }

    Value EvaluatorCoordinator::visitAnnotationNode(AnnotationNode* node)
    {
        // Annotations are metadata only - no runtime evaluation needed
        return Value();
    }
}