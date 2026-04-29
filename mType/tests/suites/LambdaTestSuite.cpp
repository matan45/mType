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
        addTestFromFile("Lambda Catch NonException",
                        errorPath + "lambdaCatchNonException.mt",
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
        addTestFromFile("Lambda Circular Reference Error",
                        errorPath + "lambdaCircularReference.mt",
                        TestType::ERROR_EXPECTED);

        // ====================================
        // Generics & Type Inference Tests
        // ====================================
        addOutputVerificationTest("Lambda Generic Type Inference",
                        passPath + "lambdaGenericTypeInference.mt");
        addOutputVerificationTest("Lambda Nested Generic Interface",
                        passPath + "lambdaNestedGenericInterface.mt");
        addOutputVerificationTest("Lambda Generic Method Inference",
                        passPath + "lambdaGenericMethodInference.mt");
        addOutputVerificationTest("Lambda Bounded Generics",
                        passPath + "lambdaBoundedGenerics.mt");
        addOutputVerificationTest("Lambda Generic Variance",
                        passPath + "lambdaGenericVariance.mt");
        addOutputVerificationTest("Lambda Generic Array Transform",
                        passPath + "lambdaGenericArrayTransform.mt");
        addOutputVerificationTest("Lambda Multiple Type Parameters",
                        passPath + "lambdaMultipleTypeParams.mt");
        addOutputVerificationTest("Lambda Generic Constraints",
                        passPath + "lambdaGenericConstraints.mt");

        // Generics error tests
        addTestFromFile("Lambda Raw Generic Type Error",
                        errorPath + "lambdaRawGenericType.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Lambda Generic Mismatch Error",
                        errorPath + "lambdaGenericMismatch.mt",
                        TestType::ERROR_EXPECTED);

        // ====================================
        // Advanced Closures Tests
        // ====================================
        addOutputVerificationTest("Lambda Double Closure",
                        passPath + "lambdaDoubleClosure.mt");
        addOutputVerificationTest("Lambda Recursive Closure",
                        passPath + "lambdaRecursiveClosure.mt");
        addOutputVerificationTest("Lambda Mutable Capture",
                        passPath + "lambdaMutableCapture.mt");
        addOutputVerificationTest("Lambda Nested Class Capture",
                        passPath + "lambdaNestedClassCapture.mt");
        addOutputVerificationTest("Lambda Capture Final",
                        passPath + "lambdaCaptureFinal.mt");
        addOutputVerificationTest("Lambda Closure Modification",
                        passPath + "lambdaClosureModification.mt");
        addOutputVerificationTest("Lambda Capture In Loop",
                        passPath + "lambdaCaptureInLoop.mt");

        // ====================================
        // Return Types Tests
        // ====================================
        addOutputVerificationTest("Lambda Implicit Void Return",
                        passPath + "lambdaImplicitVoidReturn.mt");
        addOutputVerificationTest("Lambda Multiple Return Paths",
                        passPath + "lambdaMultipleReturnPaths.mt");
        addOutputVerificationTest("Lambda Null Return",
                        passPath + "lambdaNullReturn.mt");
        addOutputVerificationTest("Lambda Subtype Return",
                        passPath + "lambdaSubtypeReturn.mt");
        addOutputVerificationTest("Lambda Array Return",
                        passPath + "lambdaArrayReturn.mt");

        // Return types error tests
        addTestFromFile("Lambda Missing Return Error",
                        errorPath + "lambdaMissingReturn.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Lambda Conflicting Returns Error",
                        errorPath + "lambdaConflictingReturns.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Lambda Closure Shadowing",
                        errorPath + "lambdaClosureShadowing.mt",
                        TestType::ERROR_EXPECTED);

        // ====================================
        // Exception Handling Tests
        // ====================================
        addOutputVerificationTest("Lambda Exception Compatibility",
                        passPath + "lambdaExceptionCompatibility.mt");
        addOutputVerificationTest("Lambda Finally With Return",
                        passPath + "lambdaFinallyWithReturn.mt");
        addOutputVerificationTest("Lambda Nested Try Catch",
                        passPath + "lambdaNestedTryCatch.mt");
        addOutputVerificationTest("Lambda Exception In Parameter",
                        passPath + "lambdaExceptionInParameter.mt");
        addOutputVerificationTest("Lambda Resource Cleanup",
                        passPath + "lambdaResourceCleanup.mt");

        // ====================================
        // Scope & Visibility Tests
        // ====================================
        addOutputVerificationTest("Lambda Private Member Access",
                        passPath + "lambdaPrivateMemberAccess.mt");
        addOutputVerificationTest("Lambda Protected Access",
                        passPath + "lambdaProtectedAccess.mt");
        addOutputVerificationTest("Lambda Anonymous Block Lambda",
                        passPath + "lambdaAnonymousBlockLambda.mt");
        addOutputVerificationTest("Lambda In Constructor",
                        passPath + "lambdaInConstructor.mt");
        addOutputVerificationTest("Lambda In Static Initializer",
                        passPath + "lambdaInStaticInitializer.mt");
        addOutputVerificationTest("Lambda Access Enclosing Private",
                        passPath + "lambdaAccessEnclosingPrivate.mt");

        // ====================================
        // Composition Tests
        // ====================================
        addOutputVerificationTest("Lambda Function Composition",
                        passPath + "lambdaFunctionComposition.mt");
        addOutputVerificationTest("Lambda Currying",
                        passPath + "lambdaCurrying.mt");
        addOutputVerificationTest("Lambda Pipeline",
                        passPath + "lambdaPipeline.mt");
        addOutputVerificationTest("Lambda Callback Chain",
                        passPath + "lambdaCallbackChain.mt");
        addOutputVerificationTest("Lambda Conditional Chain",
                        passPath + "lambdaConditionalChain.mt");

        // ====================================
        // Special Values Tests
        // ====================================
        addOutputVerificationTest("Lambda Capture Null",
                        passPath + "lambdaCaptureNull.mt");
        addOutputVerificationTest("Lambda Null Parameter",
                        passPath + "lambdaNullParameter.mt");
        addOutputVerificationTest("Lambda Null Lambda Variable",
                        passPath + "lambdaNullLambdaVariable.mt");
        addOutputVerificationTest("Lambda Zero Length Array",
                        passPath + "lambdaZeroLengthArray.mt");
        addOutputVerificationTest("Lambda Division By Zero",
                        passPath + "lambdaDivisionByZero.mt");
        addOutputVerificationTest("Lambda String Edge Cases",
                        passPath + "lambdaStringEdgeCases.mt");

        // ====================================
        // Performance Tests
        // ====================================
        addOutputVerificationTest("Lambda Deep Nesting",
                        passPath + "lambdaDeepNesting.mt");
        addOutputVerificationTest("Lambda Tight Loop Performance",
                        passPath + "lambdaTightLoopPerformance.mt");

        // ====================================
        // EDGE CASE TESTS - empty/zero/curry/this-capture
        // ====================================
        addOutputVerificationTest("Empty Lambda Body",
                        passPath + "emptyLambdaBody.mt");
        addOutputVerificationTest("Lambda Returning This",
                        passPath + "lambdaReturningThis.mt");
        addOutputVerificationTest("Lambda Zero Params",
                        passPath + "lambdaZeroParams.mt");
        addOutputVerificationTest("Nested Lambda Curry",
                        passPath + "nestedLambdaCurry.mt");

        // MYT-215: capturing a mutated variable in a lambda inside a loop is
        // rejected at compile time (Java-style "must be effectively final").
        addTestFromFile("Lambda Capture Mutable Loop Var Error (MYT-215)",
                        errorPath + "lambdaCaptureMutableLoopVar.mt",
                        TestType::ERROR_EXPECTED);
    }
}