#pragma once
#include "../value/ValueType.hpp"
#include "../ast/NodeClassesDeclaration.hpp"
#include <string>

namespace evaluator
{
    using namespace value;
    using namespace ast::nodes::statements;
    using namespace ast::nodes::functions;
    
    class Evaluator;
    
    class StatementEvaluator
    {
    private:
        Evaluator* mainEvaluator;
        bool breakFlag;
        bool continueFlag;
        
    public:
        explicit StatementEvaluator(Evaluator* evaluator);
        ~StatementEvaluator() = default;
        
        // Statement evaluation methods
        Value evaluateProgramNode(ProgramNode* node);
        Value evaluateBlockNode(BlockNode* node);
        Value evaluateDeclarationNode(DeclarationNode* node);
        Value evaluateAssignmentNode(AssignmentNode* node);
        Value evaluateMemberAssignmentNode(MemberAssignmentNode* node);
        Value evaluateIfNode(IfNode* node);
        Value evaluateWhileNode(WhileNode* node);
        Value evaluateDoWhileNode(DoWhileNode* node);
        Value evaluateForNode(ForNode* node);
        Value evaluateBreakNode(BreakNode* node);
        Value evaluateContinueNode(ContinueNode* node);
        Value evaluateSwitchNode(SwitchNode* node);
        Value evaluateCaseNode(CaseNode* node);
        Value evaluateDefaultCaseNode(DefaultCaseNode* node);
        Value evaluateImportNode(ImportNode* node);
        Value evaluateFunctionNode(FunctionNode* node);
        Value evaluateReturnNode(ReturnNode* node);
        Value evaluateNativeFunctionNode(NativeFunctionNode* node);
        
        // Helper methods
        bool shouldBreakOrContinue() const;
        void handleBreak();
        void handleContinue();
        bool isBreaking() const;
        bool isContinuing() const;
        void resetLoopFlags();
        
        // Helper for value comparison
        bool compareValues(const Value& left, const Value& right);
        
        // Helper to get ValueType from a Value
        ValueType getValueType(const Value& value);
        
        // Helper to convert ValueType to string for error messages
        std::string valueTypeToString(ValueType type);
    };
}
