#include "AwaitTestSuite.hpp"
#include "../../constants/ExecutionMode.hpp"

namespace tests::testSuite
{
    using namespace testFramework;
    using namespace constants;

    void AwaitTestSuite::setupTests()
    {
        // Async/await works in both BYTECODE_VM and AST_INTERPRETER modes
        // Default to BYTECODE_VM, but can be overridden with --bytecode flag
        setExecutionModeForAll(ExecutionMode::BYTECODE_VM);
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
    }
}
