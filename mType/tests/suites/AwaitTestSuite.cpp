#include "AwaitTestSuite.hpp"
#include "../../constants/ExecutionMode.hpp"

namespace tests::testSuite
{
    using namespace testFramework;
    using namespace constants;

    void AwaitTestSuite::setupTests()
    {
        // Async/await functionality tests (using bytecode VM mode)
        // Basic async/await functionality tests
        addOutputVerificationTest("Basic Async Function",
                        passPath + "basicAsyncFunction.mt");
        addOutputVerificationTest("Async With Arrays",
                        passPath + "asyncWithArrays.mt");
        addOutputVerificationTest("Nested Async Calls",
                        passPath + "nestedAsyncCalls.mt");
        addOutputVerificationTest("Async Methods",
                        passPath + "asyncMethods.mt");
        addOutputVerificationTest("Async With Generics",
                        passPath + "asyncWithGenerics.mt");
        addOutputVerificationTest("Multiple Awaits",
                        passPath + "multipleAwaits.mt");
        addOutputVerificationTest("Async Void Return",
                        passPath + "asyncVoidReturn.mt");

        // Advanced async/await features
        addOutputVerificationTest("Async Inheritance With Super",
                        passPath + "asyncInheritanceWithSuper.mt");
        addOutputVerificationTest("Async Interface Methods",
                        passPath + "asyncInterfaceMethods.mt");
        addOutputVerificationTest("Async Lambda Functions",
                        passPath + "asyncLambdaFunctions.mt");
        addOutputVerificationTest("Awaiting Resolved Promise",
                        passPath + "awaitingResolvedPromise.mt");
        addOutputVerificationTest("Promise Storage Edge Cases",
                        passPath + "promiseStorageEdgeCases.mt");

        // Async with exception handling (try-catch-finally)
        addOutputVerificationTest("Async Try-Catch Basic",
                        passPath + "asyncTryCatchBasic.mt");
        addOutputVerificationTest("Async Finally Return Order",
                        passPath + "asyncFinallyReturnOrder.mt");
        addOutputVerificationTest("Async Nested Try-Catch",
                        passPath + "asyncNestedTryCatch.mt");

        // === NEW EDGE CASE TESTS ===
        // Advanced async control flow scenarios

        addOutputVerificationTest("Async Control Flow Comprehensive",
                        passPath + "asyncControlFlowComprehensive.mt");

        // Race Condition & Concurrency Tests (8 tests)
        addOutputVerificationTest("Async Parallel Promises",
                        passPath + "asyncParallelPromises.mt");
        addOutputVerificationTest("Async Promise Array",
                        passPath + "asyncPromiseArray.mt");
        addOutputVerificationTest("Async Execution Order",
                        passPath + "asyncExecutionOrder.mt");
        addOutputVerificationTest("Async Concurrent Modification",
                        passPath + "asyncConcurrentModification.mt");
        addOutputVerificationTest("Async Nested Concurrency",
                        passPath + "asyncNestedConcurrency.mt");
        addOutputVerificationTest("Async Promise Chaining",
                        passPath + "asyncPromiseChaining.mt");
        addOutputVerificationTest("Async Conditional Promises",
                        passPath + "asyncConditionalPromises.mt");
        addOutputVerificationTest("Async Promise Reassignment",
                        passPath + "asyncPromiseReassignment.mt");

        // Exception & Error Handling Tests (10 tests)
        addOutputVerificationTest("Async Exception Propagation",
                        passPath + "asyncExceptionPropagation.mt");
        addOutputVerificationTest("Async Nested Exception Handling",
                        passPath + "asyncNestedExceptionHandling.mt");
        addOutputVerificationTest("Async Exception In Constructor",
                        passPath + "asyncExceptionInConstructor.mt");
        addOutputVerificationTest("Async Rethrow Exception",
                        passPath + "asyncRethrowException.mt");
        addOutputVerificationTest("Async Multiple Exception Handlers",
                        passPath + "asyncMultipleExceptionHandlers.mt");
        addOutputVerificationTest("Async Exception In Finally",
                        passPath + "asyncExceptionInFinally.mt");
        addOutputVerificationTest("Async Complex Call Stack Exception",
                        passPath + "asyncComplexCallStackException.mt");
        addOutputVerificationTest("Async Exception After Partial Execution",
                        passPath + "asyncExceptionAfterPartialExecution.mt");

        // Complex Expression Tests (8 tests)
        addOutputVerificationTest("Await In Binary Expression",
                        passPath + "awaitInBinaryExpression.mt");
        addOutputVerificationTest("Await In Method Chain",
                        passPath + "awaitInMethodChain.mt");
        addOutputVerificationTest("Await In Array Access",
                        passPath + "awaitInArrayAccess.mt");
        addOutputVerificationTest("Await In Ternary",
                        passPath + "awaitInTernary.mt");
        addOutputVerificationTest("Await As Argument",
                        passPath + "awaitAsArgument.mt");
        addOutputVerificationTest("Await In Complex Expression",
                        passPath + "awaitInComplexExpression.mt");
        addOutputVerificationTest("Await In Concatenation",
                        passPath + "awaitInConcatenation.mt");
        addOutputVerificationTest("Await In New Expression",
                        passPath + "awaitInNewExpression.mt");

        // Recursive & Control Flow Tests (10 tests)
        addOutputVerificationTest("Async Direct Recursion",
                        passPath + "asyncDirectRecursion.mt");
        addOutputVerificationTest("Async Mutual Recursion",
                        passPath + "asyncMutualRecursion.mt");
        addOutputVerificationTest("Async Await In Loop Condition",
                        passPath + "asyncAwaitInLoopCondition.mt");
        addOutputVerificationTest("Async Await In For Loop",
                        passPath + "asyncAwaitInForLoop.mt");
        addOutputVerificationTest("Async Await In While Loop",
                        passPath + "asyncAwaitInWhileLoop.mt");
        addOutputVerificationTest("Async Break In Loop",
                        passPath + "asyncBreakInLoop.mt");
        addOutputVerificationTest("Async Continue In Loop",
                        passPath + "asyncContinueInLoop.mt");
        addOutputVerificationTest("Async Nested Loops",
                        passPath + "asyncNestedLoops.mt");
        addOutputVerificationTest("Async Switch Statement",
                        passPath + "asyncSwitchStatement.mt");

        // Memory & Resource Tests (6 tests)
        addOutputVerificationTest("Async Promise Garbage Collection",
                        passPath + "asyncPromiseGarbageCollection.mt");
        addOutputVerificationTest("Async Circular Promise Reference",
                        passPath + "asyncCircularPromiseReference.mt");
        addOutputVerificationTest("Async Large Promise Chain",
                        passPath + "asyncLargePromiseChain.mt");
        addOutputVerificationTest("Async Promise In Class Field",
                        passPath + "asyncPromiseInClassField.mt");
        addOutputVerificationTest("Async Promise In Static Field",
                        passPath + "asyncPromiseInStaticField.mt");
        addOutputVerificationTest("Async Memory Stress",
                        passPath + "asyncMemoryStress.mt");

        // Modifiers & Type System Tests (8 tests)
        addOutputVerificationTest("Async Private Method",
                        passPath + "asyncPrivateMethod.mt");
        addOutputVerificationTest("Async Protected Method",
                        passPath + "asyncProtectedMethod.mt");
        addOutputVerificationTest("Async Override Method",
                        passPath + "asyncOverrideMethod.mt");
        addOutputVerificationTest("Async Static Method Variant",
                        passPath + "asyncStaticMethodVariant.mt");
        addOutputVerificationTest("Async Promise Covariance",
                        passPath + "asyncPromiseCovariance.mt");
        addOutputVerificationTest("Async Generic Promise",
                        passPath + "asyncGenericPromise.mt");
        addOutputVerificationTest("Async Promise Cast",
                        passPath + "asyncPromiseCast.mt");

        // Error tests (expected to fail)
        addTestFromFile("Async Without Promise Return",
                        errorPath + "asyncWithoutPromiseReturn.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Async With Primitive Return",
                        errorPath + "asyncWithPrimitiveReturn.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Await Outside Async",
                        errorPath + "awaitOutsideAsync.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Async Method Without Promise",
                        errorPath + "asyncMethodWithoutPromise.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Async Returning Void",
                        errorPath + "asyncReturningVoid.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Async Constructor",
                        errorPath + "asyncConstructor.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Async Unhandled Exception",
                        errorPath + "asyncUnhandledException.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Async Wrong Exception Type",
                        errorPath + "asyncWrongExceptionType.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Async Infinite Recursion",
                        errorPath + "asyncInfiniteRecursion.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Async Final Method Error",
                        errorPath + "asyncFinalMethodError.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Await In Constructor Error",
                        errorPath + "awaitInConstructorError.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Await In Static Initializer",
                        errorPath + "awaitInStaticInitializer.mt",
                        TestType::ERROR_EXPECTED);

        // Edge Context Tests (2 more pass tests)
        addOutputVerificationTest("Async Main Function",
                        passPath + "asyncMainFunction.mt");
        addOutputVerificationTest("Async Anonymous Block",
                        passPath + "asyncAnonymousBlock.mt");
    }
}
