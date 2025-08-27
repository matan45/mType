#include "ControlFlowTestSuite.hpp"

namespace tests::testSuite
{
    using namespace testFramework;
    
    void ControlFlowTestSuite::setupTests()
    {
        // Function-related tests
        addTestFromFile("Type Script Style Functions",
                        passPath + "typeScriptStyleFunctions.mt");
        addTestFromFile("Complex Functions with New Syntax",
                        passPath + "complexFunctionsNewSyntax.mt");
        addTestFromFile("Void Functions",
                        passPath + "voidFunctions.mt");
        addTestFromFile("Mixed Parameter Types",
                        passPath + "mixedParameterTypes.mt");
        addTestFromFile("Nested Function Calls",
                        passPath + "nestedFunctionCalls.mt");
        addTestFromFile("Recursion with New Syntax",
                        passPath + "recursionNewSyntax.mt");

        // Control flow tests
        addTestFromFile("Break Statement",
                        passPath + "breakStatement.mt");
        addTestFromFile("Continue Statement",
                        passPath + "continueStatement.mt");
        addTestFromFile("Nested Loops with Break Continue",
                        passPath + "nestedLoopsWithBreakContinue.mt");
        addTestFromFile("Nested Loop Break Continue Limits",
                        passPath + "nestedLoopBreakContinueLimits.mt");

        // Final variables tests
        addTestFromFile("Final Variables",
                        passPath + "finalVariables.mt");
        addTestFromFile("Final in Loops",
                        passPath + "finalInLoops.mt");
        addTestFromFile("Final in Functions",
                        passPath + "finalInFunctions.mt");

        // Switch-case tests
        addTestFromFile("Switch Case with Default",
                        passPath + "switchCaseDefault.mt");
        addTestFromFile("Switch Extreme Cases",
                        passPath + "switchExtremeCases.mt");
        addTestFromFile("Switch with Break Statements",
                        passPath + "switchWithBreak.mt");
        addTestFromFile("Switch Nested in Loops",
                        passPath + "switchNestedInLoops.mt");
        addTestFromFile("Switch with Complex Expressions",
                        passPath + "switchComplexExpressions.mt");
        addTestFromFile("Switch Edge Case Values",
                        passPath + "switchEdgeCaseValues.mt");

        // Ternary operator tests
        addTestFromFile("Ternary Operator Basic",
                        passPath + "ternaryOperatorBasic.mt");
        addTestFromFile("Ternary Operator Nested",
                        passPath + "ternaryOperatorNested.mt");
        addTestFromFile("Ternary Operator Nesting Limits",
                        passPath + "ternaryOperatorNestingLimits.mt");

        // Boolean expression tests
        addTestFromFile("Complex Boolean Expressions",
                        passPath + "complexBooleanExpressions.mt");

        // Compound assignment tests
        addTestFromFile("Compound Assignment Operators",
                        passPath + "compoundAssignmentOperators.mt");

        // Arithmetic edge case tests
        addTestFromFile("Modulo with Negative Operands",
                        passPath + "moduloNegativeOperands.mt");

        // Recursion edge case tests
        addTestFromFile("Mutual Recursion",
                        passPath + "mutualRecursion.mt");

        // Error tests (expected to fail)
        addTestFromFile("Final Variable Reassignment Error",
                        errorPath + "finalVariableReassignment.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Static Function Parameters Error",
                        errorPath + "staticFunctionParameters.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Final Function Parameters Error",
                        errorPath + "finalFunctionParameters.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Static For Loop Variable Error",
                        errorPath + "staticForLoopVariable.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Final For Loop Variable Error",
                        errorPath + "finalForLoopVariable.mt",
                        TestType::ERROR_EXPECTED);
    }
}
