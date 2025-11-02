#include "ErrorTestSuite.hpp"

namespace tests::testSuite
{
    using namespace testFramework;

    void ErrorTestSuite::setupTests()
    {
        // Basic passing tests for error handling
        addOutputVerificationTest("Valid Error Reporting Code",
                        passPath + "basicErrorReportingValid.mt");
        addOutputVerificationTest("Valid Function Declaration",
                        passPath + "validFunctionDeclaration.mt");
        addOutputVerificationTest("Valid Class Usage",
                        passPath + "validClassUsage.mt");
        addOutputVerificationTest("Valid Type Operations",
                        passPath + "validTypeOperations.mt");
        addOutputVerificationTest("Valid Scope Usage",
                        passPath + "validScopeUsage.mt");
        addOutputVerificationTest("Valid Inner Scope Blocks",
                        passPath + "validInnerScope.mt");

        // Lexer error tests (expected to fail) - FIXED: No longer causes infinite loop
        addTestFromFile("Lexer Unterminated String Error",
                        errorPath + "lexerUnteminatedString.mt",
                        TestType::ERROR_EXPECTED);

        // Parser error tests (expected to fail)
        addTestFromFile("Parser Missing Semicolon Error",
                        errorPath + "parserMissingSemicolon.mt",
                        TestType::ERROR_EXPECTED);

        // Syntax error tests (expected to fail)
        addTestFromFile("Syntax Missing Semicolon Error",
                        errorPath + "syntaxMissingSemicolon.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Syntax Unmatched Parentheses Error",
                        errorPath + "syntaxUnmatchedParentheses.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Syntax Invalid Function Name Error",
                        errorPath + "syntaxInvalidFunctionName.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Syntax Missing Return Type Error",
                        errorPath + "syntaxMissingReturnType.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Nested Function Declaration Error",
                        errorPath + "nestedFunctionDeclaration.mt",
                        TestType::ERROR_EXPECTED);

        // Semantic error tests (expected to fail)
        addTestFromFile("Semantic Undefined Variable Error",
                        errorPath + "semanticUndefinedVariable.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Semantic Variable Redefinition Error",
                        errorPath + "semanticVariableRedefinition.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Undefined Variable Reassignment Error",
                        errorPath + "undefinedVariableReassignment.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Semantic Wrong Parameter Count Error",
                        errorPath + "semanticWrongParameterCount.mt",
                        TestType::ERROR_EXPECTED);

        // Runtime error tests (expected to fail)
        addTestFromFile("Runtime Division By Zero Error",
                        errorPath + "runtimeDivisionByZero.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Runtime Null Pointer Dereference Error",
                        errorPath + "runtimeNullPointerDereference.mt",
                        TestType::ERROR_EXPECTED);

        addTestFromFile("Missing new Operator For Object Creation",
                        errorPath + "missingNew.mt",
                        TestType::ERROR_EXPECTED);

        // Type error tests (expected to fail)
        addTestFromFile("Type Assignment Mismatch Error",
                        errorPath + "typeAssignmentMismatch.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Assign Null To Final",
                        errorPath + "assignNullToFinal.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Type Invalid Operation Error",
                        errorPath + "typeInvalidOperation.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Type Return Type Mismatch Error",
                        errorPath + "typeReturnTypeMismatch.mt",
                        TestType::ERROR_EXPECTED);

        // Class error tests (expected to fail)
        addTestFromFile("Class Undefined Class Error",
                        errorPath + "classUndefinedClass.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Class Undefined Member Error",
                        errorPath + "classUndefinedMember.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Class Constructor Parameter Mismatch Error",
                        errorPath + "classConstructorParameterMismatch.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Class Modifying Final Field Error",
                        errorPath + "classModifyingFinalField.mt",
                        TestType::ERROR_EXPECTED);

        // Scope error tests (expected to fail)
        addTestFromFile("Scope Variable Out Of Scope Error",
                        errorPath + "scopeVariableOutOfScope.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Scope Function Parameter Shadowing Error",
                        errorPath + "scopeFunctionParameterShadowing.mt",
                        TestType::ERROR_EXPECTED);

        // Import error tests (expected to fail)
        addTestFromFile("Import Non Existent File Error",
                        errorPath + "importNonExistentFile.mt",
                        TestType::ERROR_EXPECTED);

        // Enhanced exception location tests (expected to fail with location info)
        addTestFromFile("Enhanced Division By Zero Location Error",
                        errorPath + "enhancedDivisionByZeroLocation.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Enhanced Type Mismatch Location Error",
                        errorPath + "enhancedTypeMismatchLocation.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Enhanced Undefined Function Location Error",
                        errorPath + "enhancedUndefinedFunctionLocation.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Enhanced Static Method Error Location",
                        errorPath + "enhancedStaticMethodErrorLocation.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Enhanced Argument Mismatch Location Error",
                        errorPath + "enhancedArgumentMismatchLocation.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Enhanced Final Variable Modification Location Error",
                        errorPath + "enhancedFinalVariableModificationLocation.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Enhanced Null Pointer Location Error",
                        errorPath + "enhancedNullPointerLocation.mt",
                        TestType::ERROR_EXPECTED);

        // Static method validation error tests (expected to fail)
        addTestFromFile("Static Method Access This Error",
                        errorPath + "staticMethodAccessThis.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Static Method Access Instance Field Error",
                        errorPath + "staticMethodAccessInstanceField.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Static Method Access Instance Member Error",
                        errorPath + "staticMethodAccessInstanceMember.mt",
                        TestType::ERROR_EXPECTED);

        // === IDENTIFIER VALIDATION TESTS ===
        // These tests verify that identifiers with special characters are rejected

        addTestFromFile("Invalid Variable Name with Special Character Error",
                        errorPath + "invalidVariableNameSpecialChar.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Invalid Class Name with Special Character Error",
                        errorPath + "invalidClassNameSpecialChar.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Invalid Interface Name with Special Character Error",
                        errorPath + "invalidInterfaceNameSpecialChar.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Invalid Function Name with Special Character Error",
                        errorPath + "invalidFunctionNameSpecialChar.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Invalid Field Name with Special Character Error",
                        errorPath + "invalidFieldNameSpecialChar.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Invalid Parameter Name with Special Character Error",
                        errorPath + "invalidParameterNameSpecialChar.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Invalid Method Name with Special Character Error",
                        errorPath + "invalidMethodNameSpecialChar.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Invalid Static Variable Name with Special Character Error",
                        errorPath + "invalidStaticVarNameSpecialChar.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Invalid Import Symbol Name with Special Character Error",
                        errorPath + "invalidImportSymbolNameSpecialChar.mt",
                        TestType::ERROR_EXPECTED);

        // === DUPLICATE TYPE DECLARATION TESTS ===
        // These tests verify that duplicate class/interface names are rejected
        addTestFromFile("Duplicate Class Name Error",
                        errorPath + "duplicateClassName.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Duplicate Interface Name Error",
                        errorPath + "duplicateInterfaceName.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Duplicate Class and Interface Name Error",
                        errorPath + "duplicateClassInterfaceName.mt",
                        TestType::ERROR_EXPECTED);

        // === DUPLICATE FUNCTION DECLARATION TESTS ===
        // These tests verify that duplicate global function names are rejected
        addTestFromFile("Duplicate Function Name Error",
                        errorPath + "duplicateFunctionName.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Duplicate Native Function Name Error",
                        errorPath + "duplicateNativeFunctionName.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Duplicate Function and Native Function Name Error",
                        errorPath + "duplicateFunctionNativeName.mt",
                        TestType::ERROR_EXPECTED);

        // === DUPLICATE METHOD DECLARATION TESTS ===
        // These tests verify that duplicate method names within a class are rejected
        addTestFromFile("Duplicate Instance Method Name Error",
                        errorPath + "duplicateInstanceMethodName.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Duplicate Static Method Name Error",
                        errorPath + "duplicateStaticMethodName.mt",
                        TestType::ERROR_EXPECTED);

        // ====================================
        // NEW EDGE CASE TESTS (70 tests)
        // ====================================

        // === ADVANCED FLOW (8 tests) ===
        addOutputVerificationTest("Error Exception In Catch",
                        passPath + "errorExceptionInCatch_pass.mt");
        addOutputVerificationTest("Error Multiple Finally",
                        passPath + "errorMultipleFinally_pass.mt");
        addOutputVerificationTest("Error Try Catch Finally Order",
                        passPath + "errorTryCatchFinallyOrder_pass.mt");
        addOutputVerificationTest("Error Break In Finally",
                        passPath + "errorBreakInFinally_pass.mt");
        addOutputVerificationTest("Error Continue In Finally",
                        passPath + "errorContinueInFinally_pass.mt");
        addOutputVerificationTest("Error Return In Finally",
                        passPath + "errorReturnInFinally_pass.mt");
        addOutputVerificationTest("Error Nested Try Deep",
                        passPath + "errorNestedTryDeep_pass.mt");
        addOutputVerificationTest("Error Finally Execution Order",
                        passPath + "errorFinallyExecutionOrder_pass.mt");

        // === TYPE MATCHING (7 tests) ===
        addOutputVerificationTest("Error Catch Block Order",
                        passPath + "errorCatchBlockOrder_pass.mt");
        addOutputVerificationTest("Error Catch Specific Before General",
                        passPath + "errorCatchSpecificBeforeGeneral_pass.mt");
        addTestFromFile("Error Catch Unreachable",
                        errorPath + "errorCatchUnreachable_error.mt",
                        TestType::ERROR_EXPECTED);
        addOutputVerificationTest("Error Abstract Exception",
                        passPath + "errorAbstractException_pass.mt");
        addOutputVerificationTest("Error Generic Exception Type",
                        passPath + "errorGenericExceptionType_pass.mt");
        addOutputVerificationTest("Error Interface Exception",
                        passPath + "errorInterfaceException_pass.mt");
        addTestFromFile("Error Throw Null",
                        errorPath + "errorThrowNull_error.mt",
                        TestType::ERROR_EXPECTED);

        // === SPECIAL CONSTRUCTS (10 tests) ===
        addOutputVerificationTest("Error In Constructor",
                        passPath + "errorInConstructor_pass.mt");
        addOutputVerificationTest("Error In Static Initializer",
                        passPath + "errorInStaticInitializer_pass.mt");
        addOutputVerificationTest("Error In Field Initializer",
                        passPath + "errorInFieldInitializer_pass.mt");
        addOutputVerificationTest("Error In Destructor",
                        passPath + "errorInDestructor_pass.mt");
        addOutputVerificationTest("Error In Loop",
                        passPath + "errorInLoop_pass.mt");
        addOutputVerificationTest("Error In Switch",
                        passPath + "errorInSwitch_pass.mt");
        addOutputVerificationTest("Error In Ternary",
                        passPath + "errorInTernary_pass.mt");
        addOutputVerificationTest("Error In Array Initializer",
                        passPath + "errorInArrayInitializer_pass.mt");
        addOutputVerificationTest("Error In Method Reference",
                        passPath + "errorInMethodReference_pass.mt");
        addOutputVerificationTest("Error In Anonymous Class",
                        passPath + "errorInAnonymousClass_pass.mt");

        // === GENERICS (6 tests) ===
        addOutputVerificationTest("Error Generic Exception Catch",
                        passPath + "errorGenericExceptionCatch_pass.mt");
        addOutputVerificationTest("Error Bounded Exception Type",
                        passPath + "errorBoundedExceptionType_pass.mt");
        addOutputVerificationTest("Error Generic Method Exception",
                        passPath + "errorGenericMethodException_pass.mt");
        addOutputVerificationTest("Error Nested Generic Exception",
                        passPath + "errorNestedGenericException_pass.mt");
        addTestFromFile("Error Generic Array Exception",
                        errorPath + "errorGenericArrayException_error.mt",
                        TestType::ERROR_EXPECTED);
        addOutputVerificationTest("Error Wildcard Exception",
                        passPath + "errorWildcardException_pass.mt");

        // === INTERFACES (5 tests) ===
        addOutputVerificationTest("Error Interface Throws Clause",
                        passPath + "errorInterfaceThrowsClause_pass.mt");
        addOutputVerificationTest("Error Override Exception Covariant",
                        passPath + "errorOverrideExceptionCovariant_pass.mt");
        addTestFromFile("Error Override Exception Invalid",
                        errorPath + "errorOverrideExceptionInvalid_error.mt",
                        TestType::ERROR_EXPECTED);
        addOutputVerificationTest("Error Multiple Interface Exceptions",
                        passPath + "errorMultipleInterfaceExceptions_pass.mt");
        addOutputVerificationTest("Error Diamond Interface Exception",
                        passPath + "errorDiamondInterfaceException_pass.mt");

        // === LAMBDAS (5 tests) ===
        addOutputVerificationTest("Error Lambda Capture Exception",
                        passPath + "errorLambdaCaptureException_pass.mt");
        addOutputVerificationTest("Error Lambda Throws",
                        passPath + "errorLambdaThrows_pass.mt");
        addOutputVerificationTest("Error Lambda Exception Propagation",
                        passPath + "errorLambdaExceptionPropagation_pass.mt");
        addOutputVerificationTest("Error Lambda Try Catch",
                        passPath + "errorLambdaTryCatch_pass.mt");
        addOutputVerificationTest("Error Nested Lambda Exception",
                        passPath + "errorNestedLambdaException_pass.mt");

        // === ASYNC (8 tests) ===
        addOutputVerificationTest("Error Async Await Exception",
                        passPath + "errorAsyncAwaitException_pass.mt");
        addOutputVerificationTest("Error Promise Rejection",
                        passPath + "errorPromiseRejection_pass.mt");
        addOutputVerificationTest("Error Promise Chain Exception",
                        passPath + "errorPromiseChainException_pass.mt");
        addOutputVerificationTest("Error Promise All Rejection",
                        passPath + "errorPromiseAllRejection_pass.mt");
        addOutputVerificationTest("Error Promise Race Exception",
                        passPath + "errorPromiseRaceException_pass.mt");
        addOutputVerificationTest("Error Async Parallel Exception",
                        passPath + "errorAsyncParallelException_pass.mt");
        addOutputVerificationTest("Error Async Callback Exception",
                        passPath + "errorAsyncCallbackException_pass.mt");
        addOutputVerificationTest("Error Event Loop Exception",
                        passPath + "errorEventLoopException_pass.mt");

        // === RESOURCE MANAGEMENT (6 tests) ===
        addOutputVerificationTest("Error Try With Resources",
                        passPath + "errorTryWithResources_pass.mt");
        addOutputVerificationTest("Error Resource Cleanup Failure",
                        passPath + "errorResourceCleanupFailure_pass.mt");
        addOutputVerificationTest("Error Multiple Resources",
                        passPath + "errorMultipleResources_pass.mt");
        addOutputVerificationTest("Error Resource Exception Suppressed",
                        passPath + "errorResourceExceptionSuppressed_pass.mt");
        addOutputVerificationTest("Error RAII Pattern",
                        passPath + "errorRAIIPattern_pass.mt");
        addOutputVerificationTest("Error Transaction Rollback",
                        passPath + "errorTransactionRollback_pass.mt");

        // === STACK TRACES (5 tests) ===
        addOutputVerificationTest("Error Stack Trace Accuracy",
                        passPath + "errorStackTraceAccuracy_pass.mt");
        addOutputVerificationTest("Error Stack Trace Deep",
                        passPath + "errorStackTraceDeep_pass.mt");
        addOutputVerificationTest("Error Stack Trace Recursive",
                        passPath + "errorStackTraceRecursive_pass.mt");
        addOutputVerificationTest("Error Stack Trace Async",
                        passPath + "errorStackTraceAsync_pass.mt");
        addOutputVerificationTest("Error Stack Trace Lambda",
                        passPath + "errorStackTraceLambda_pass.mt");

        // === PERFORMANCE (4 tests) ===
        addOutputVerificationTest("Error Exception Creation Cost",
                        passPath + "errorExceptionCreationCost_pass.mt");
        addOutputVerificationTest("Error Deep Nesting Performance",
                        passPath + "errorDeepNestingPerformance_pass.mt");
        addOutputVerificationTest("Error Many Exceptions",
                        passPath + "errorManyExceptions_pass.mt");
        addOutputVerificationTest("Error Large Exception Payload",
                        passPath + "errorLargeExceptionPayload_pass.mt");

        // === ANNOTATIONS (4 tests) ===
        addOutputVerificationTest("Error Throw Annotation Single",
                        passPath + "errorThrowAnnotationSingle_pass.mt");
        addOutputVerificationTest("Error Throw Annotation Multiple",
                        passPath + "errorThrowAnnotationMultiple_pass.mt");
        addTestFromFile("Error Throw Annotation Mismatch",
                        errorPath + "errorThrowAnnotationMismatch_error.mt",
                        TestType::ERROR_EXPECTED);
        addOutputVerificationTest("Error Throw Annotation Inherited",
                        passPath + "errorThrowAnnotationInherited_pass.mt");

        // === RECOVERY (5 tests) ===
        addOutputVerificationTest("Error Retry Mechanism",
                        passPath + "errorRetryMechanism_pass.mt");
        addOutputVerificationTest("Error Circuit Breaker",
                        passPath + "errorCircuitBreaker_pass.mt");
        addOutputVerificationTest("Error Fallback Value",
                        passPath + "errorFallbackValue_pass.mt");
        addOutputVerificationTest("Error Exponential Backoff",
                        passPath + "errorExponentialBackoff_pass.mt");
        addOutputVerificationTest("Error Custom Recovery Strategy",
                        passPath + "errorCustomRecoveryStrategy_pass.mt");

        // === INTEROP (2 tests) ===
        addOutputVerificationTest("Error Cross Module Exception",
                        passPath + "errorCrossModuleException_pass.mt");
        addOutputVerificationTest("Error Module Initialization Exception",
                        passPath + "errorModuleInitException_pass.mt");

        // Test that static and instance methods with same name are allowed (different namespaces)
        addOutputVerificationTest("Allow Static And Instance Method Same Name",
                        passPath + "allowStaticAndInstanceSameName.mt");
    }
}
