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
    }
}