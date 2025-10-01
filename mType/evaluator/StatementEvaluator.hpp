#pragma once
#include "base/EvaluationContext.hpp"
#include "managers/ControlFlowManager.hpp"
#include "utils/ValueConverter.hpp"
#include "../ast/NodeClassesDeclaration.hpp"
#include "../ast/nodes/statements/BreakNode.hpp"
#include "../errors/SourceLocation.hpp"
#include <memory>

// Forward declarations for specialized handlers
namespace evaluator {
namespace statements {
    class LoopEvaluator;
    class DeclarationHandler;
    class ControlFlowHandler;
    class ImportAndFunctionHandler;
}
}

namespace evaluator
{
    using namespace base;
    using namespace managers;
    using namespace utils;
    using namespace value;
    using namespace ast::nodes::statements;
    using namespace ast::nodes::functions;

    /**
     * @brief Refactored Statement Evaluator following SOLID principles
     * - Single Responsibility: Only handles statement evaluation
     * - Open/Closed: Extensible through composition
     * - Liskov Substitution: Implements IStatementEvaluator interface
     * - Interface Segregation: Uses specialized interfaces
     * - Dependency Inversion: Depends on abstractions
     */
    class StatementEvaluator
    {
    private:
        std::shared_ptr<EvaluationContext> context;
        std::unique_ptr<ControlFlowManager> flowManager;

        // Specialized statement handlers
        std::unique_ptr<statements::LoopEvaluator> loopEvaluator;
        std::unique_ptr<statements::DeclarationHandler> declarationHandler;
        std::unique_ptr<statements::ControlFlowHandler> controlFlowHandler;
        std::unique_ptr<statements::ImportAndFunctionHandler> importAndFunctionHandler;

        // Forward declarations for circular dependency resolution
        class ExpressionEvaluator* exprEvaluator;
        class ObjectEvaluator* objEvaluator;

        // Helper method for import evaluation
        void evaluateRecursively(ASTNode* node);

    public:
        explicit StatementEvaluator(std::shared_ptr<EvaluationContext> ctx);
        ~StatementEvaluator();

        // IEvaluator interface implementation
        Value evaluate(ASTNode* node);
        bool canHandle(ASTNode* node) const;

        // IStatementEvaluator interface implementation
        bool shouldBreakOrContinue() const;
        void handleBreak();
        void handleContinue();
        void resetLoopFlags();

        // Loop depth management
        void enterLoop();
        void exitLoop();
        bool isInLoop() const;
        void enterBreakableContext(); // Generic method for entering any breakable context
        void exitBreakableContext();  // Generic method for exiting any breakable context
        bool isInBreakableContext() const; // Generic method for checking if break is valid

        // Statement evaluation methods
        Value evaluateProgramNode(ProgramNode* node);
        Value evaluateBlockNode(BlockNode* node);
        Value evaluateDeclarationNode(DeclarationNode* node);
        Value evaluateAssignmentNode(AssignmentNode* node);
        Value evaluateIfNode(IfNode* node);
        Value evaluateWhileNode(WhileNode* node);
        Value evaluateDoWhileNode(DoWhileNode* node);
        Value evaluateForNode(ForNode* node);
        Value evaluateForEachNode(ForEachNode* node);
        Value evaluateBreakNode(BreakNode* node);
        Value evaluateContinueNode(ContinueNode* node);
        Value evaluateSwitchNode(SwitchNode* node);
        Value evaluateCaseNode(CaseNode* node);
        Value evaluateDefaultCaseNode(DefaultCaseNode* node);
        Value evaluateImportNode(ImportNode* node);
        Value evaluateFunctionNode(FunctionNode* node);
        Value evaluateReturnNode(ReturnNode* node);
        Value evaluateNativeFunctionNode(ast::nodes::statements::NativeFunctionNode* node);

        // Dependency injection for cross-evaluator communication
        void setExpressionEvaluator(ExpressionEvaluator* evaluator);
        void setObjectEvaluator(ObjectEvaluator* evaluator);

    private:
        // Helper methods
        bool isStatementNode(ASTNode* node) const;
        Value executeStatementList(const std::vector<std::unique_ptr<ASTNode>>& statements);
        void validateVariableDeclaration(DeclarationNode* node);
        void validateAssignmentAsDeclaration(AssignmentNode* node);
        void validateTypeAssignment(ValueType expectedType, const Value& value,
                                    const std::string& variableName,
                                    const SourceLocation& location);
        void validateTypeAssignment(ValueType expectedType, const Value& value,
                                    const std::string& variableName,
                                    const SourceLocation& location,
                                    const std::string& expectedClassName);
        void validateClassExists(const std::string& className, const SourceLocation& location);
        bool isValidTypeConversion(ValueType from, ValueType to);
        bool isGenericTypeCompatible(const std::string& actualClassName,
                                   const std::string& expectedClassName);
        void validateObjectTypeCompatibility(const Value& value, const std::string& variableName,
                                             const SourceLocation& location,
                                             const std::string& expectedClassName);

    public:
        // Lambda-to-interface conversion (moved to public for ObjectEvaluator access)
        Value convertLambdaToInterface(const Value& lambdaValue, const std::string& interfaceName, const SourceLocation& location = SourceLocation());
    };
}
