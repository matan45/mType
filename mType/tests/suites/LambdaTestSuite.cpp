#include "LambdaTestSuite.hpp"

namespace tests::testSuite
{
    using namespace testFramework;

    void LambdaTestSuite::setupTests()
    {
        // Basic lambda functionality tests
        addOutputVerificationTest("Basic Expression Lambda",
                        passPath + "basicExpressionLambda.mt");
        addOutputVerificationTest("Basic Block Lambda",
                        passPath + "basicBlockLambda.mt");

        // Functional interface tests
        addOutputVerificationTest("Runnable Interface",
                        passPath + "runnableInterface.mt");
        addOutputVerificationTest("Supplier Interface",
                        passPath + "supplierInterface.mt");
        addOutputVerificationTest("Consumer Interface",
                        passPath + "consumerInterface.mt");
        addOutputVerificationTest("Predicate Interface",
                        passPath + "predicateInterface.mt");
        addOutputVerificationTest("Interface as Parameter",
                        passPath + "interfaceAsParameter.mt");

        addOutputVerificationTest("lambda interface test",
                        passPath + "lambda_interface_test.mt");

        // Advanced lambda tests
        addOutputVerificationTest("Multi-Parameter Lambda",
                        passPath + "multiParameterLambda.mt");
        addOutputVerificationTest("Complex Block Lambda",
                        passPath + "complexBlockLambda.mt");

        // Closure capture tests
        addOutputVerificationTest("Closure Variable Capture",
                        passPath + "closureCapture.mt");
        addOutputVerificationTest("Nested Closure",
                        passPath + "nestedClosure.mt");
        addOutputVerificationTest("Closure Block Lambda",
                        passPath + "closureBlockLambda.mt");
        addOutputVerificationTest("Closure Capture Edge Cases",
                        passPath + "closureCaptureEdgeCases.mt");

        // Class integration tests
        addOutputVerificationTest("Lambda Instance Field Access",
                        passPath + "classInstanceFieldAccess.mt");
        addOutputVerificationTest("Lambda This Keyword Access",
                        passPath + "classThisKeyword.mt");
        addOutputVerificationTest("Lambda Static Field Access",
                        passPath + "classStaticAccess.mt");
        addOutputVerificationTest("Lambda Method Parameter Capture",
                        passPath + "classMethodParameters.mt");
        addOutputVerificationTest("Lambda Complex Class Integration",
                        passPath + "classComplexIntegration.mt");
        addOutputVerificationTest("Lambda Super Access",
                        passPath + "lambdaSuperAccess.mt");

        // Memory management and advanced tests
        addOutputVerificationTest("Lambda Interface Memory Management",
                        passPath + "test_lambda_interface_memory_mgmt.mt");

        // Lambda lifecycle edge case tests
        addOutputVerificationTest("Lambda Lifecycle Basic",
                        passPath + "lambdaLifecycleBasic.mt");
        addOutputVerificationTest("Lambda Lifecycle Scope",
                        passPath + "lambdaLifecycleScope.mt");
        addOutputVerificationTest("Lambda Lifecycle Stress",
                        passPath + "lambdaLifecycleStress.mt");
        addOutputVerificationTest("Lambda Lifecycle Expired",
                        passPath + "lambdaLifecycleExpired.mt");

        // === NEW EDGE CASE TESTS ===
        // Advanced lambda and control flow scenarios

        addOutputVerificationTest("Lambda With Break",
                        passPath + "lambdaWithBreak.mt");
        addOutputVerificationTest("Lambda Try Catch Finally",
                        passPath + "lambdaTryCatchFinally.mt");
        addOutputVerificationTest("Lambda With Throw",
                        passPath + "lambdaWithThrow.mt");

        // Error tests (expected to fail)
        addTestFromFile("Lambda Parameter Type Mismatch",
                        errorPath + "lambdaParameterTypeMismatch.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Lambda Return Type Mismatch",
                        errorPath + "lambdaReturnTypeMismatch.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Lambda Parameter Count Mismatch",
                        errorPath + "lambdaParameterCountMismatch.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Lambda to Non-Functional Interface",
                        errorPath + "lambdaToNonFunctionalInterface.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Lambda to Undefined Interface",
                        errorPath + "lambdaToUndefinedInterface.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Lambda Syntax Error",
                        errorPath + "lambdaSyntaxError.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Lambda Reassignment Not Allowed",
                        errorPath + "lambdaReassignmentNotAllowed.mt",
                        TestType::ERROR_EXPECTED);

        // === NEW EDGE CASE ERROR TESTS ===
        // Advanced lambda error scenarios

        addTestFromFile("Break In Lambda Error",
                        errorPath + "breakInLambdaError.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Continue In Lambda Error",
                        errorPath + "continueInLambdaError.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Lambda Super In Static Context Error",
                        errorPath + "lambdaSuperStaticContext.mt",
                        TestType::ERROR_EXPECTED);
    }
}