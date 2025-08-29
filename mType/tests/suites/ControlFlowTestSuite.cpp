#include "ControlFlowTestSuite.hpp"

namespace tests::testSuite
{
    using namespace testFramework;
    
    void ControlFlowTestSuite::setupTests()
    {
        // Function-related tests
        addOutputVerificationTest("Type Script Style Functions",
                        passPath + "typeScriptStyleFunctions.mt");
        addOutputVerificationTest("Complex Functions with New Syntax",
                        passPath + "complexFunctionsNewSyntax.mt");
        addOutputVerificationTest("Void Functions",
                        passPath + "voidFunctions.mt");
        addOutputVerificationTest("Mixed Parameter Types",
                        passPath + "mixedParameterTypes.mt");
        addOutputVerificationTest("Nested Function Calls",
                        passPath + "nestedFunctionCalls.mt");
        addOutputVerificationTest("Recursion with New Syntax",
                        passPath + "recursionNewSyntax.mt");

        // Control flow tests
        addOutputVerificationTest("Break Statement",
                        passPath + "breakStatement.mt");
        addOutputVerificationTest("Continue Statement",
                        passPath + "continueStatement.mt");
        addOutputVerificationTest("Nested Loops with Break Continue",
                        passPath + "nestedLoopsWithBreakContinue.mt");
        /*addOutputVerificationTest("Nested Loop Break Continue Limits",
                        passPath + "nestedLoopBreakContinueLimits.mt");*/

        // Final variables tests
        addOutputVerificationTest("Final Variables",
                        passPath + "finalVariables.mt");
        addOutputVerificationTest("Final in Loops",
                        passPath + "finalInLoops.mt");
        addOutputVerificationTest("Final in Functions",
                        passPath + "finalInFunctions.mt");

        // Switch-case tests
        addOutputVerificationTest("Switch Case with Default",
                        passPath + "switchCaseDefault.mt");
        addOutputVerificationTest("Switch Extreme Cases",
                        passPath + "switchExtremeCases.mt");
        addOutputVerificationTest("Switch with Break Statements",
                        passPath + "switchWithBreak.mt");
        addOutputVerificationTest("Switch Nested in Loops",
                        passPath + "switchNestedInLoops.mt");
        addOutputVerificationTest("Switch with Complex Expressions",
                        passPath + "switchComplexExpressions.mt");
        addOutputVerificationTest("Switch Edge Case Values",
                        passPath + "switchEdgeCaseValues.mt");

        // Ternary operator tests
        addOutputVerificationTest("Ternary Operator Basic",
                        passPath + "ternaryOperatorBasic.mt");
        addOutputVerificationTest("Ternary Operator Nested",
                        passPath + "ternaryOperatorNested.mt");
        addOutputVerificationTest("Ternary Operator Nesting Limits",
                        passPath + "ternaryOperatorNestingLimits.mt");

        // Boolean expression tests
        addOutputVerificationTest("Complex Boolean Expressions",
                        passPath + "complexBooleanExpressions.mt");

        // Compound assignment tests
        addOutputVerificationTest("Compound Assignment Operators",
                        passPath + "compoundAssignmentOperators.mt");

        // Arithmetic edge case tests
        addOutputVerificationTest("Modulo with Negative Operands",
                        passPath + "moduloNegativeOperands.mt");

        // Recursion edge case tests
        addOutputVerificationTest("Mutual Recursion",
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
