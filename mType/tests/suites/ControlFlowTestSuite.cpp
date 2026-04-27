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
        // MYT-112: bare `return;` accepted in constructors and remains
        // accepted in void functions/methods.
        addOutputVerificationTest("Void Early Return (regression guard)",
                        passPath + "voidEarlyReturn_pass.mt");
        addOutputVerificationTest("Void Return In Constructor",
                        passPath + "voidReturnInCtor_pass.mt");
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
        addTestFromFile("Nested Loop Break Continue Limits",
                        passPath + "nestedLoopBreakContinueLimits.mt");

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

        // Match statement (pattern matching) tests
        addOutputVerificationTest("Match Value Pattern",
                        passPath + "matchValuePattern_pass.mt");
        addOutputVerificationTest("Match Type Pattern",
                        passPath + "matchTypePattern_pass.mt");
        addOutputVerificationTest("Match Null Pattern",
                        passPath + "matchNullPattern_pass.mt");
        addOutputVerificationTest("Match Default Case",
                        passPath + "matchDefaultCase_pass.mt");
        addOutputVerificationTest("Match Block Body",
                        passPath + "matchBlockBody_pass.mt");
        addOutputVerificationTest("Match Nested",
                        passPath + "matchNested_pass.mt");
        addOutputVerificationTest("Match Type Inheritance",
                        passPath + "matchTypeInheritance_pass.mt");

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

        // Spaced operator tests
        addOutputVerificationTest("Spaced Operators",
                        passPath + "spacedOperators.mt");

        // Arithmetic edge case tests
        addOutputVerificationTest("Modulo with Negative Operands",
                        passPath + "moduloNegativeOperands.mt");

        // Recursion edge case tests
        addOutputVerificationTest("Mutual Recursion",
                        passPath + "mutualRecursion.mt");

        // === NEW EDGE CASE TESTS ===
        // Advanced control flow scenarios

        addOutputVerificationTest("Nested Lambda Return",
                        passPath + "nestedLambdaReturn.mt");
        addOutputVerificationTest("Finally Overrides Return",
                        passPath + "finallyOverridesReturn.mt");

        // ====================================
        // ADDITIONAL TESTS - Files that exist but weren't registered
        // ====================================

        // === ASYNC TESTS (6 tests) ===
        addOutputVerificationTest("Async Basic Await",
                        passPath + "asyncBasicAwait_pass.mt");
        addOutputVerificationTest("Async In Loop",
                        passPath + "asyncInLoop_pass.mt");
        addOutputVerificationTest("Async Nested Await",
                        passPath + "asyncNestedAwait_pass.mt");
        addOutputVerificationTest("Async Parallel",
                        passPath + "asyncParallel_pass.mt");
        addOutputVerificationTest("Async Promise Chain",
                        passPath + "asyncPromiseChain_pass.mt");
        addOutputVerificationTest("Async Exception Handling",
                        passPath + "asyncExceptionHandling_pass.mt");

        // === CONDITIONAL TESTS (7 tests) ===
        addOutputVerificationTest("Conditional Dangling Else",
                        passPath + "conditionalDanglingElse_pass.mt");
        addOutputVerificationTest("Conditional Deep Else If",
                        passPath + "conditionalDeepElseIf_pass.mt");
        addOutputVerificationTest("Conditional Empty Blocks",
                        passPath + "conditionalEmptyBlocks_pass.mt");
        addOutputVerificationTest("Conditional In Switch",
                        passPath + "conditionalInSwitch_pass.mt");
        addOutputVerificationTest("Conditional Nested Complex",
                        passPath + "conditionalNestedComplex_pass.mt");
        addOutputVerificationTest("Conditional No Else",
                        passPath + "conditionalNoElse_pass.mt");
        addOutputVerificationTest("Conditional Redundant Checks",
                        passPath + "conditionalRedundantChecks_pass.mt");
        addOutputVerificationTest("Conditional Short Circuit Division",
                        passPath + "conditionalShortCircuitDiv_pass.mt");

        // === EDGE TESTS (5 tests) ===
        addOutputVerificationTest("Edge Empty Function",
                        passPath + "edgeEmptyFunction_pass.mt");
        addOutputVerificationTest("Edge Max Nesting Depth",
                        passPath + "edgeMaxNestingDepth_pass.mt");
        addOutputVerificationTest("Edge Mixed Indentation",
                        passPath + "edgeMixedIndentation_pass.mt");
        addOutputVerificationTest("Edge Single Statement Blocks",
                        passPath + "edgeSingleStatementBlocks_pass.mt");
        addOutputVerificationTest("Edge Stack Depth Limit",
                        passPath + "edgeStackDepthLimit_pass.mt");

        // === EXCEPTION TESTS (15 tests) ===
        addOutputVerificationTest("Exception Break In Try",
                        passPath + "exceptionBreakInTry_pass.mt");
        addOutputVerificationTest("Exception Catch No Var",
                        passPath + "exceptionCatchNoVar_pass.mt");
        addOutputVerificationTest("Exception Continue In Catch",
                        passPath + "exceptionContinueInCatch_pass.mt");
        addOutputVerificationTest("Exception In Loop",
                        passPath + "exceptionInLoop_pass.mt");
        addOutputVerificationTest("Exception Nested Finally",
                        passPath + "exceptionNestedFinally_pass.mt");
        addOutputVerificationTest("Exception Nested Try Catch",
                        passPath + "exceptionNestedTryCatch_pass.mt");
        addOutputVerificationTest("Exception Propagation",
                        passPath + "exceptionPropagation_pass.mt");
        addOutputVerificationTest("Exception Resource Cleanup",
                        passPath + "exceptionResourceCleanup_pass.mt");
        addOutputVerificationTest("Exception Rethrow",
                        passPath + "exceptionRethrow_pass.mt");
        addOutputVerificationTest("Exception Return In Finally",
                        passPath + "exceptionReturnInFinally_pass.mt");
        addOutputVerificationTest("Exception Try Catch Multiple",
                        passPath + "exceptionTryCatchMultiple_pass.mt");
        addOutputVerificationTest("Exception Try Finally",
                        passPath + "exceptionTryFinally_pass.mt");

        // === LOOP TESTS (9 tests) ===
        addOutputVerificationTest("Loop Counter Overflow",
                        passPath + "loopCounterOverflow_pass.mt");
        addOutputVerificationTest("Loop For Each",
                        passPath + "loopForEach_pass.mt");
        addOutputVerificationTest("Loop Labeled Break",
                        passPath + "loopLabeledBreak_pass.mt");
        addOutputVerificationTest("Loop Labeled Continue",
                        passPath + "loopLabeledContinue_pass.mt");
        addOutputVerificationTest("Loop Multiple Exit Conditions",
                        passPath + "loopMultipleExitConditions_pass.mt");
        addOutputVerificationTest("Loop Nested Complex",
                        passPath + "loopNestedComplex_pass.mt");
        addOutputVerificationTest("Loop Reverse Iteration",
                        passPath + "loopReverseIteration_pass.mt");
        addOutputVerificationTest("Loop Side Effect Condition",
                        passPath + "loopSideEffectCondition_pass.mt");
        addOutputVerificationTest("Loop Zero Iterations",
                        passPath + "loopZeroIterations_pass.mt");

        // === MODERN TESTS (5 tests) ===
        addOutputVerificationTest("Modern Early Return",
                        passPath + "modernEarlyReturn_pass.mt");
        addOutputVerificationTest("Modern Guard Clauses",
                        passPath + "modernGuardClauses_pass.mt");
        addOutputVerificationTest("Modern Null Coalescing",
                        passPath + "modernNullCoalescing_pass.mt");
        addOutputVerificationTest("Modern Null Safe Navigation",
                        passPath + "modernNullSafeNavigation_pass.mt");
        addOutputVerificationTest("Modern Ternary Chains",
                        passPath + "modernTernaryChains_pass.mt");

        // === RECURSION TESTS (10 tests) ===
        addOutputVerificationTest("Recursion Indirect Through",
                        passPath + "recursionIndirectThrough_pass.mt");
        addOutputVerificationTest("Recursion Lambda Self",
                        passPath + "recursionLambdaSelf_pass.mt");
        addOutputVerificationTest("Recursion Memoization",
                        passPath + "recursionMemoization_pass.mt");
        addOutputVerificationTest("Recursion Multiple Base Cases",
                        passPath + "recursionMultipleBaseCases_pass.mt");
        addOutputVerificationTest("Recursion Tail Optimization",
                        passPath + "recursionTailOptimization_pass.mt");
        addOutputVerificationTest("Recursion Self Direct Call",
                        passPath + "recursionSelfDirectCall_pass.mt");

        // === RETURN TESTS (5 tests) ===
        addOutputVerificationTest("Return Chained Calls",
                        passPath + "returnChainedCalls_pass.mt");
        addOutputVerificationTest("Return Complex Expression",
                        passPath + "returnComplexExpression_pass.mt");
        addOutputVerificationTest("Return Conditional",
                        passPath + "returnConditional_pass.mt");
        addOutputVerificationTest("Return In Nested Blocks",
                        passPath + "returnInNestedBlocks_pass.mt");
        addOutputVerificationTest("Return Multiple Paths",
                        passPath + "returnMultiplePaths_pass.mt");

        // === SWITCH TESTS (7 tests) ===
        addOutputVerificationTest("Switch Fall Through",
                        passPath + "switchFallThrough_pass.mt");
        addOutputVerificationTest("Switch In Loop",
                        passPath + "switchInLoop_pass.mt");
        addOutputVerificationTest("Switch Many Cases",
                        passPath + "switchManyCases_pass.mt");
        addOutputVerificationTest("Switch No Default",
                        passPath + "switchNoDefault_pass.mt");
        addOutputVerificationTest("Switch Only Default",
                        passPath + "switchOnlyDefault_pass.mt");
        addOutputVerificationTest("Switch String Values",
                        passPath + "switchStringValues_pass.mt");

        // ====================================
        // NEW EDGE CASE TESTS (70 tests)
        // COMMENTED OUT - Test files were not created
        // ====================================

        /*
        // === LOOP EDGE CASES (10 tests) ===
        addOutputVerificationTest("Loop Infinite While",
                        passPath + "loopInfiniteWhile_pass.mt");
        addOutputVerificationTest("Loop Zero Iterations",
                        passPath + "loopZeroIterations_pass.mt");
        addOutputVerificationTest("Loop Labeled Break",
                        passPath + "loopLabeledBreak_pass.mt");
        addOutputVerificationTest("Loop Labeled Continue",
                        passPath + "loopLabeledContinue_pass.mt");
        addOutputVerificationTest("Loop Nested Labeled",
                        passPath + "loopNestedLabeled_pass.mt");
        addOutputVerificationTest("Loop Break in Finally",
                        passPath + "loopBreakInFinally_pass.mt");
        addOutputVerificationTest("Loop Continue Lambda",
                        passPath + "loopContinueLambda_pass.mt");
        addOutputVerificationTest("Loop Iterator Concurrent Modification",
                        passPath + "loopIteratorConcurrentMod_pass.mt");
        addOutputVerificationTest("Loop Empty Body",
                        passPath + "loopEmptyBody_pass.mt");
        addOutputVerificationTest("Loop Max Iterations Stress",
                        passPath + "loopMaxIterationsStress_pass.mt");

        // === CONDITIONAL EDGE CASES (8 tests) ===
        addOutputVerificationTest("Conditional Deep Else If",
                        passPath + "conditionalDeepElseIf_pass.mt");
        addOutputVerificationTest("Conditional Empty Blocks",
                        passPath + "conditionalEmptyBlocks_pass.mt");
        addOutputVerificationTest("Conditional Short Circuit And",
                        passPath + "conditionalShortCircuitAnd_pass.mt");
        addOutputVerificationTest("Conditional Short Circuit Or",
                        passPath + "conditionalShortCircuitOr_pass.mt");
        addOutputVerificationTest("Conditional Nested Max Depth",
                        passPath + "conditionalNestedMaxDepth_pass.mt");
        addOutputVerificationTest("Conditional All Branches Return",
                        passPath + "conditionalAllBranchesReturn_pass.mt");
        addOutputVerificationTest("Conditional Boolean Expression Complex",
                        passPath + "conditionalBoolExprComplex_pass.mt");
        addOutputVerificationTest("Conditional Side Effects",
                        passPath + "conditionalSideEffects_pass.mt");

        // === SWITCH EDGE CASES (8 tests) ===
        addOutputVerificationTest("Switch String Values",
                        passPath + "switchStringValues_pass.mt");
        addOutputVerificationTest("Switch Many Cases",
                        passPath + "switchManyCases_pass.mt");
        addOutputVerificationTest("Switch Fall Through Multiple",
                        passPath + "switchFallThroughMultiple_pass.mt");
        addOutputVerificationTest("Switch Nested Deep",
                        passPath + "switchNestedDeep_pass.mt");
        addOutputVerificationTest("Switch No Default",
                        passPath + "switchNoDefault_pass.mt");
        addOutputVerificationTest("Switch Empty Case",
                        passPath + "switchEmptyCase_pass.mt");
        addOutputVerificationTest("Switch Return Each Case",
                        passPath + "switchReturnEachCase_pass.mt");
        addOutputVerificationTest("Switch Expression Result",
                        passPath + "switchExpressionResult_pass.mt");

        // === RETURN STATEMENT EDGE CASES (6 tests) ===
        addOutputVerificationTest("Return Multiple Paths",
                        passPath + "returnMultiplePaths_pass.mt");
        addOutputVerificationTest("Return Deeply Nested",
                        passPath + "returnDeeplyNested_pass.mt");
        addOutputVerificationTest("Return In Finally",
                        passPath + "returnInFinally_pass.mt");
        addOutputVerificationTest("Return In Loop",
                        passPath + "returnInLoop_pass.mt");
        addOutputVerificationTest("Return Ternary Expression",
                        passPath + "returnTernaryExpr_pass.mt");
        addOutputVerificationTest("Return Lambda Implicit",
                        passPath + "returnLambdaImplicit_pass.mt");

        // === RECURSION EDGE CASES (6 tests) ===
        addOutputVerificationTest("Recursion Tail Optimizable",
                        passPath + "recursionTailOptimizable_pass.mt");
        addOutputVerificationTest("Recursion Deep Stack",
                        passPath + "recursionDeepStack_pass.mt");
        addOutputVerificationTest("Recursion Mutual Three Functions",
                        passPath + "recursionMutualThree_pass.mt");
        addOutputVerificationTest("Recursion Indirect",
                        passPath + "recursionIndirect_pass.mt");
        addOutputVerificationTest("Recursion With Memoization",
                        passPath + "recursionWithMemoization_pass.mt");
        addOutputVerificationTest("Recursion Lambda Recursive",
                        passPath + "recursionLambdaRecursive_pass.mt");

        // === ASYNC & CONCURRENCY (6 tests) ===
        addOutputVerificationTest("Async Await Basic",
                        passPath + "asyncAwaitBasic_pass.mt");
        addOutputVerificationTest("Async Chain Promises",
                        passPath + "asyncChainPromises_pass.mt");
        addOutputVerificationTest("Async Multiple Awaits",
                        passPath + "asyncMultipleAwaits_pass.mt");
        addOutputVerificationTest("Async Parallel Execution",
                        passPath + "asyncParallelExecution_pass.mt");
        addOutputVerificationTest("Async Exception Handling",
                        passPath + "asyncExceptionHandling_pass.mt");
        addOutputVerificationTest("Async Race Condition Test",
                        passPath + "asyncRaceCondition_pass.mt");

        // === EDGE CONDITIONS (5 tests) ===
        addOutputVerificationTest("Edge Empty Function",
                        passPath + "edgeEmptyFunction_pass.mt");
        addOutputVerificationTest("Edge Max Nesting Depth",
                        passPath + "edgeMaxNestingDepth_pass.mt");
        addOutputVerificationTest("Edge Single Statement Blocks",
                        passPath + "edgeSingleStatementBlocks_pass.mt");
        addOutputVerificationTest("Edge All Control Flow Combined",
                        passPath + "edgeAllControlFlowCombined_pass.mt");
        addOutputVerificationTest("Edge Stack Depth Limit",
                        passPath + "edgeStackDepthLimit_pass.mt");

        // === MODERN FEATURES (5 tests) ===
        addOutputVerificationTest("Modern Null Coalescing Chain",
                        passPath + "modernNullCoalescingChain_pass.mt");
        addOutputVerificationTest("Modern Optional Chaining",
                        passPath + "modernOptionalChaining_pass.mt");
        addOutputVerificationTest("Modern Guard Clauses",
                        passPath + "modernGuardClauses_pass.mt");
        addOutputVerificationTest("Modern Pattern Matching",
                        passPath + "modernPatternMatching_pass.mt");
        addOutputVerificationTest("Modern Ternary Chain",
                        passPath + "modernTernaryChain_pass.mt");

        // === ERROR CASES (4 tests) ===
        addTestFromFile("Error Break Outside Loop",
                        errorPath + "errorBreakOutsideLoop_error.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Error Continue Outside Loop",
                        errorPath + "errorContinueOutsideLoop_error.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Error Return Outside Function",
                        errorPath + "errorReturnOutsideFunction_error.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Error Unreachable Code",
                        errorPath + "errorUnreachableCode_error.mt",
                        TestType::ERROR_EXPECTED);
        */

        // === CROSS-FEATURE EDGE CASES ===
        addOutputVerificationTest("Match Inside Async Function",
                        passPath + "matchInsideAsync_pass.mt");
        addOutputVerificationTest("Switch On Interpolated String",
                        passPath + "switchOnInterpolatedString_pass.mt");

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

        // === EDGE CASE TESTS - finally semantics / nesting / masking ===
        addOutputVerificationTest("Final Return Overrides Try Return",
                        passPath + "finalReturnOverridesTryReturn.mt");
        addOutputVerificationTest("Throw Inside Catch Replaces Original",
                        passPath + "throwInsideCatchReplacesOriginal.mt");
        addOutputVerificationTest("Finally Masks Try Exception",
                        passPath + "finallyMasksTryException.mt");
        addOutputVerificationTest("Deeply Nested Blocks",
                        passPath + "deeplyNestedBlocks.mt");
    }
}
