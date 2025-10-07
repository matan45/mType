#include "Evaluator.hpp"
#include "../runtimeTypes/klass/ObjectInstance.hpp"

namespace evaluator
{
    using namespace runtimeTypes::klass;
    
    Evaluator::Evaluator(std::shared_ptr<Environment> environment)
    {
        coordinator = std::make_unique<EvaluatorCoordinator>(environment);
    }

    Evaluator::~Evaluator() = default;

    Value Evaluator::evaluate(ASTNode* node)
    {
        return coordinator->evaluate(node);
    }

    // ASTVisitor interface implementation - delegate to coordinator
    Value Evaluator::visitProgramNode(ProgramNode* node)
    {
        return coordinator->visitProgramNode(node);
    }

    Value Evaluator::visitBlockNode(BlockNode* node)
    {
        return coordinator->visitBlockNode(node);
    }

    Value Evaluator::visitFloatNode(FloatNode* node)
    {
        return coordinator->visitFloatNode(node);
    }

    Value Evaluator::visitIntegerNode(IntegerNode* node)
    {
        return coordinator->visitIntegerNode(node);
    }

    Value Evaluator::visitStringNode(StringNode* node)
    {
        return coordinator->visitStringNode(node);
    }

    Value Evaluator::visitBoolNode(BoolNode* node)
    {
        return coordinator->visitBoolNode(node);
    }

    Value Evaluator::visitVariableNode(VariableNode* node)
    {
        return coordinator->visitVariableNode(node);
    }

    Value Evaluator::visitDeclarationNode(DeclarationNode* node)
    {
        return coordinator->visitDeclarationNode(node);
    }

    Value Evaluator::visitAssignmentNode(AssignmentNode* node)
    {
        return coordinator->visitAssignmentNode(node);
    }

    Value Evaluator::visitBinaryOpNode(BinaryOpNode* node)
    {
        return coordinator->visitBinaryOpNode(node);
    }

    Value Evaluator::visitTernaryOpNode(TernaryOpNode* node)
    {
        return coordinator->visitTernaryOpNode(node);
    }

    Value Evaluator::visitUnaryOpNode(UnaryOpNode* node)
    {
        return coordinator->visitUnaryOpNode(node);
    }

    Value Evaluator::visitIfNode(IfNode* node)
    {
        return coordinator->visitIfNode(node);
    }

    Value Evaluator::visitWhileNode(WhileNode* node)
    {
        return coordinator->visitWhileNode(node);
    }

    Value Evaluator::visitDoWhileNode(DoWhileNode* node)
    {
        return coordinator->visitDoWhileNode(node);
    }

    Value Evaluator::visitForNode(ForNode* node)
    {
        return coordinator->visitForNode(node);
    }

    Value Evaluator::visitBreakNode(BreakNode* node)
    {
        return coordinator->visitBreakNode(node);
    }

    Value Evaluator::visitContinueNode(ContinueNode* node)
    {
        return coordinator->visitContinueNode(node);
    }

    Value Evaluator::visitFunctionNode(FunctionNode* node)
    {
        return coordinator->visitFunctionNode(node);
    }

    Value Evaluator::visitFunctionCallNode(FunctionCallNode* node)
    {
        return coordinator->visitFunctionCallNode(node);
    }

    Value Evaluator::visitReturnNode(ReturnNode* node)
    {
        return coordinator->visitReturnNode(node);
    }

    Value Evaluator::visitSwitchNode(SwitchNode* node)
    {
        return coordinator->visitSwitchNode(node);
    }

    Value Evaluator::visitCaseNode(CaseNode* node)
    {
        return coordinator->visitCaseNode(node);
    }

    Value Evaluator::visitImportNode(ImportNode* node)
    {
        return coordinator->visitImportNode(node);
    }

    Value Evaluator::visitDefaultCaseNode(DefaultCaseNode* node)
    {
        return coordinator->visitDefaultCaseNode(node);
    }

    Value Evaluator::visitNativeFunctionNode(NativeFunctionNode* node)
    {
        return coordinator->visitNativeFunctionNode(node);
    }

    Value Evaluator::visitNullNode(NullNode* node)
    {
        return coordinator->visitNullNode(node);
    }

    Value Evaluator::visitMemberAssignmentNode(MemberAssignmentNode* node)
    {
        return coordinator->visitMemberAssignmentNode(node);
    }

    Value Evaluator::visitIndexAssignmentNode(IndexAssignmentNode* node)
    {
        return coordinator->visitIndexAssignmentNode(node);
    }

    Value Evaluator::visitMethodCallNode(MethodCallNode* node)
    {
        return coordinator->visitMethodCallNode(node);
    }

    Value Evaluator::visitMemberAccessNode(MemberAccessNode* node)
    {
        return coordinator->visitMemberAccessNode(node);
    }

    Value Evaluator::visitNewNode(NewNode* node)
    {
        return coordinator->visitNewNode(node);
    }

    Value Evaluator::visitMethodNode(MethodNode* node)
    {
        return coordinator->visitMethodNode(node);
    }

    Value Evaluator::visitConstructorNode(ConstructorNode* node)
    {
        return coordinator->visitConstructorNode(node);
    }

    Value Evaluator::visitFieldNode(FieldNode* node)
    {
        return coordinator->visitFieldNode(node);
    }

    Value Evaluator::visitClassNode(ClassNode* node)
    {
        return coordinator->visitClassNode(node);
    }

    Value Evaluator::visitInterfaceNode(InterfaceNode* node)
    {
        return coordinator->visitInterfaceNode(node);
    }

    Value Evaluator::visitCastExpression(CastExpression* node)
    {
        return coordinator->visitCastExpression(node);
    }

    Value Evaluator::visitInstanceOfExpression(InstanceOfExpression* node)
    {
        return coordinator->visitInstanceOfExpression(node);
    }

    // Helper method implementations - delegate to coordinator
    std::shared_ptr<Environment> Evaluator::getEnvironment() const
    {
        return coordinator->getContext()->getEnvironment();
    }

    std::shared_ptr<base::EvaluationContext> Evaluator::getContext() const
    {
        return coordinator->getContext();
    }

    bool Evaluator::shouldReturn() const
    {
        return coordinator->shouldReturn();
    }

    void Evaluator::setReturned(bool returned)
    {
        coordinator->setReturned(returned);
    }

    Value Evaluator::getReturnValue()
    {
        return coordinator->getReturnValue();
    }

    void Evaluator::pushReturnValue(const Value& value)
    {
        coordinator->pushReturnValue(value);
    }

    std::shared_ptr<ObjectInstance> Evaluator::getCurrentInstance() const
    {
        return coordinator->getCurrentInstance();
    }

    bool Evaluator::isTruthy(const Value& value)
    {
        return coordinator->isTruthy(value);
    }

    std::string Evaluator::toString(const Value& value)
    {
        return coordinator->toString(value);
    }

    float Evaluator::toFloat(const Value& value)
    {
        return coordinator->toFloat(value);
    }

    int Evaluator::toInt(const Value& value)
    {
        return coordinator->toInt(value);
    }

    Value Evaluator::evaluateObjectMethodCall(MethodCallNode* node)
    {
        return coordinator->visitMethodCallNode(node);
    }

    Value Evaluator::evaluateObjectCreation(NewNode* node)
    {
        return coordinator->visitNewNode(node);
    }

    Value Evaluator::evaluateObjectMemberAccess(MemberAccessNode* node)
    {
        return coordinator->visitMemberAccessNode(node);
    }

    Value Evaluator::evaluateObjectMemberAssignment(MemberAssignmentNode* node)
    {
        return coordinator->visitMemberAssignmentNode(node);
    }

    Value Evaluator::callMethodOnInstance(std::shared_ptr<ObjectInstance> instance, 
                                          const std::string& methodName, const std::vector<Value>& args)
    {
        // This would need to be implemented in the object evaluator
        auto objEvaluator = coordinator->getObjectEvaluator();
        return objEvaluator->callMethod(instance, methodName, args);
    }

    // Collection visitor methods - delegate to coordinator
    Value Evaluator::visitArrayCreationNode(ArrayCreationNode* node)
    {
        return coordinator->visitArrayCreationNode(node);
    }

    Value Evaluator::visitArrayLiteralNode(ArrayLiteralNode* node)
    {
        return coordinator->visitArrayLiteralNode(node);
    }

    Value Evaluator::visitIndexAccessNode(IndexAccessNode* node)
    {
        return coordinator->visitIndexAccessNode(node);
    }

    Value Evaluator::visitForEachNode(ForEachNode* node)
    {
        return coordinator->visitForEachNode(node);
    }

    Value Evaluator::visitLambdaNode(LambdaNode* node)
    {
        return coordinator->visitLambdaNode(node);
    }

    Value Evaluator::visitSuperConstructorCallNode(SuperConstructorCallNode* node)
    {
        return coordinator->visitSuperConstructorCallNode(node);
    }

    Value Evaluator::visitSuperMethodCallNode(SuperMethodCallNode* node)
    {
        return coordinator->visitSuperMethodCallNode(node);
    }

}