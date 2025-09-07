#include "EvaluatorCoordinator.hpp"
#include "../runtimeTypes/klass/ObjectInstance.hpp"

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
    
    Value EvaluatorCoordinator::visitDeclarationNode(DeclarationNode* node)
    {
        return stmtEvaluator->evaluateDeclarationNode(node);
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
    
    Value EvaluatorCoordinator::visitNativeFunctionNode(NativeFunctionNode* node)
    {
        return stmtEvaluator->evaluateNativeFunctionNode(node);
    }
    
    Value EvaluatorCoordinator::visitNullNode(NullNode* node)
    {
        return exprEvaluator->evaluateNullNode(node);
    }
    
    Value EvaluatorCoordinator::visitMemberAssignmentNode(MemberAssignmentNode* node)
    {
        return objEvaluator->evaluateMemberAssignmentNode(node);
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
    Value EvaluatorCoordinator::visitArrayLiteralNode(ArrayLiteralNode* node)
    {
        return exprEvaluator->evaluateArrayLiteralNode(node);
    }

    Value EvaluatorCoordinator::visitMapLiteralNode(MapLiteralNode* node)
    {
        return exprEvaluator->evaluateMapLiteralNode(node);
    }

    Value EvaluatorCoordinator::visitIndexAccessNode(IndexAccessNode* node)
    {
        return exprEvaluator->evaluateIndexAccessNode(node);
    }

    Value EvaluatorCoordinator::visitForEachNode(ForEachNode* node)
    {
        return stmtEvaluator->evaluateForEachNode(node);
    }
}