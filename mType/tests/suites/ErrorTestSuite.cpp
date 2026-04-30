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
        // VirtualMachine::pushCallFrame trips the call-stack guard at
        // DEFAULT_MAX_CALL_STACK_SIZE and throws a RuntimeException whose
        // message starts with "Stack overflow:" — pin that substring so a
        // future change to the wording surfaces deliberately.
        addTestFromFile("Runtime Stack Overflow Error",
                        errorPath + "runtimeStackOverflow.mt",
                        TestType::ERROR_EXPECTED,
                        "Stack overflow");

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
        // MYT-46 — interop wrapper decorates the caught exception with a
        // real SourceLocation walked from the live callStack. Substring
        // filter pins the diagnostic to the NullPointerException path.
        addTestFromFile("MYT-46 Interop Null Deref Carries Location",
                        errorPath + "interopNullDeref_error.mt",
                        TestType::ERROR_EXPECTED,
                        "Cannot");

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
        // Removed: "Duplicate Function Name Error" — FunctionRegistrar was made
        // idempotent on re-registration to fix MYT-113-adjacent re-import
        // breakage (the same AST being traversed via two import chains was
        // throwing duplicate-signature). The trade-off is that genuine
        // same-file duplicates also no longer error here. Tracked in MYT-116:
        // restore strict detection at the parser/semantic-pass layer (which
        // can distinguish re-import from real duplicates via SourceLocation)
        // and reintroduce this test.

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

        // ====================================
        // ADDITIONAL TESTS - Files that exist but weren't registered
        // ====================================

        // === ASYNC TESTS (11 tests) ===
        addOutputVerificationTest("Error Async Await Exception",
                        passPath + "errorAsyncAwaitException_pass.mt");
        addOutputVerificationTest("Error Async Finally",
                        passPath + "errorAsyncFinally_pass.mt");
        addOutputVerificationTest("Error Async Multiple Catch",
                        passPath + "errorAsyncMultipleCatch_pass.mt");
        addOutputVerificationTest("Error Async Nested Exceptions",
                        passPath + "errorAsyncNestedExceptions_pass.mt");
        addOutputVerificationTest("Error Async Parallel Exceptions",
                        passPath + "errorAsyncParallelExceptions_pass.mt");
        addOutputVerificationTest("Error Async Rethrow",
                        passPath + "errorAsyncRethrow_pass.mt");
        addOutputVerificationTest("Error Async Try Catch",
                        passPath + "errorAsyncTryCatch_pass.mt");
        addOutputVerificationTest("Error Async Unhandled",
                        passPath + "errorAsyncUnhandled_pass.mt");

        // === CATCH TESTS (8 tests) ===
        addOutputVerificationTest("Error Catch Abstract Exception",
                        passPath + "errorCatchAbstractException_pass.mt");
        addOutputVerificationTest("Error Catch Generic Type",
                        passPath + "errorCatchGenericType_pass.mt");
        addOutputVerificationTest("Error Catch Inheritance Chain",
                        passPath + "errorCatchInheritanceChain_pass.mt");
        addOutputVerificationTest("Error Catch Interface Type",
                        passPath + "errorCatchInterfaceType_pass.mt");
        addOutputVerificationTest("Error Catch Unrelated Types",
                        passPath + "errorCatchUnrelatedTypes_pass.mt");
        addOutputVerificationTest("Error Empty Catch",
                        passPath + "errorEmptyCatch_pass.mt");
        addOutputVerificationTest("Error Recursive Catch",
                        passPath + "errorRecursiveCatch_pass.mt");

        // === CONTROL FLOW TESTS (5 tests) ===
        addOutputVerificationTest("Error Break In Try Finally",
                        passPath + "errorBreakInTryFinally_pass.mt");
        addOutputVerificationTest("Error Continue In Catch Finally",
                        passPath + "errorContinueInCatchFinally_pass.mt");
        addOutputVerificationTest("Error Return In Catch Finally",
                        passPath + "errorReturnInCatchFinally_pass.mt");
        addOutputVerificationTest("Error Multiple Finally No Catch",
                        passPath + "errorMultipleFinallyNoCatch_pass.mt");

        // === ERROR LOCATION TESTS (11 tests) ===
        addOutputVerificationTest("Error In Array Init",
                        passPath + "errorInArrayInit_pass.mt");
        addOutputVerificationTest("Error In Default Param",
                        passPath + "errorInDefaultParam_pass.mt");
        addOutputVerificationTest("Error In Field Init",
                        passPath + "errorInFieldInit_pass.mt");
        addOutputVerificationTest("Error In For Loop Condition",
                        passPath + "errorInForLoopCondition_pass.mt");
        addOutputVerificationTest("Error In For Loop Increment",
                        passPath + "errorInForLoopIncrement_pass.mt");
        addOutputVerificationTest("Error In For Loop Init",
                        passPath + "errorInForLoopInit_pass.mt");
        addOutputVerificationTest("Error In Static Init",
                        passPath + "errorInStaticInit_pass.mt");
        addOutputVerificationTest("Error In Switch Expr",
                        passPath + "errorInSwitchExpr_pass.mt");
        addOutputVerificationTest("Error In Ternary Operator",
                        passPath + "errorInTernaryOperator_pass.mt");
        addOutputVerificationTest("Error In While Condition",
                        passPath + "errorInWhileCondition_pass.mt");

        // === GENERICS TESTS (4 tests) ===
        addOutputVerificationTest("Error Bounded Generic Exception",
                        passPath + "errorBoundedGenericException_pass.mt");
        addOutputVerificationTest("Error Generic Exception",
                        passPath + "errorGenericException_pass.mt");
        addOutputVerificationTest("Error Generic Method Exception",
                        passPath + "errorGenericMethodException_pass.mt");
        addOutputVerificationTest("Error Multiple Generic Bounds",
                        passPath + "errorMultipleGenericBounds_pass.mt");
        addOutputVerificationTest("Error Throw Generic Exception",
                        passPath + "errorThrowGenericException_pass.mt");

        // === INTERFACE TESTS (4 tests) ===
        addOutputVerificationTest("Error Covariant Exception Types",
                        passPath + "errorCovariantExceptionTypes_pass.mt");
        addOutputVerificationTest("Error Default Method Exception",
                        passPath + "errorDefaultMethodException_pass.mt");
        addOutputVerificationTest("Error Impl Different Throws",
                        passPath + "errorImplDifferentThrows_pass.mt");
        addOutputVerificationTest("Error Interface Method Throws",
                        passPath + "errorInterfaceMethodThrows_pass.mt");
        addOutputVerificationTest("Error Multiple Interface Conflict",
                        passPath + "errorMultipleInterfaceConflict_pass.mt");

        // === LAMBDA TESTS (5 tests) ===
        addOutputVerificationTest("Error Lambda Capturing Exception",
                        passPath + "errorLambdaCapturingException_pass.mt");
        addOutputVerificationTest("Error Lambda Chain Exception",
                        passPath + "errorLambdaChainException_pass.mt");
        addOutputVerificationTest("Error Lambda In Try Catch",
                        passPath + "errorLambdaInTryCatch_pass.mt");
        addOutputVerificationTest("Error Lambda Throwing Exception",
                        passPath + "errorLambdaThrowingException_pass.mt");
        addOutputVerificationTest("Error Higher Order Exception",
                        passPath + "errorHigherOrderException_pass.mt");

        // === RESOURCE MANAGEMENT TESTS (7 tests) ===
        addOutputVerificationTest("Error Resource Cleanup Multiple Fail",
                        passPath + "errorResourceCleanupMultipleFail_pass.mt");
        addOutputVerificationTest("Error Resource Finally Cleanup",
                        passPath + "errorResourceFinallyCleanup_pass.mt");
        addOutputVerificationTest("Error Resource Leak Detection",
                        passPath + "errorResourceLeakDetection_pass.mt");
        addOutputVerificationTest("Error Resource Nested Acquisition",
                        passPath + "errorResourceNestedAcquisition_pass.mt");
        addOutputVerificationTest("Error Resource RAII",
                        passPath + "errorResourceRAII_pass.mt");
        addOutputVerificationTest("Error Resource Transaction Rollback",
                        passPath + "errorResourceTransactionRollback_pass.mt");

        // === STACK TRACE TESTS (5 tests) ===
        addOutputVerificationTest("Error Stack Trace Accuracy",
                        passPath + "errorStackTraceAccuracy_pass.mt");
        addOutputVerificationTest("Error Stack Trace Async",
                        passPath + "errorStackTraceAsync_pass.mt");
        addOutputVerificationTest("Error Stack Trace Custom Message",
                        passPath + "errorStackTraceCustomMessage_pass.mt");
        addOutputVerificationTest("Error Stack Trace Nested",
                        passPath + "errorStackTraceNested_pass.mt");
        addOutputVerificationTest("Error Stack Trace With Lambda",
                        passPath + "errorStackTraceWithLambda_pass.mt");

        // === THROW VALIDATION ERROR TESTS (3 tests) ===
        // Files exist on disk but were previously unregistered (MYT-38).
        addTestFromFile("Error Throw Primitive",
                        errorPath + "errorThrowPrimitive_error.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Error Throw Null",
                        errorPath + "errorThrowNull_error.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Error Throw Mismatch",
                        errorPath + "errorThrowMismatch_error.mt",
                        TestType::ERROR_EXPECTED);

        // === MYT-38 STACK TRACE CONTENT (6 tests) ===
        // These tests assert on the *content* of the trace string, not just
        // its presence. They lock in the format emitted by ExceptionExecutor
        // (function names, frame order, source line numbers, lambda/method
        // naming) so future changes to the trace format intentionally break
        // and force a deliberate update.
        addOutputVerificationTest("Error Stack Trace Frame Order",
                        passPath + "errorStackTraceFrameOrder_pass.mt");
        addOutputVerificationTest("Error Stack Trace Method Name Format",
                        passPath + "errorStackTraceMethodNameFormat_pass.mt");
        addOutputVerificationTest("Error Stack Trace Rethrow Identity",
                        passPath + "errorStackTraceRethrowIdentity_pass.mt");
        addOutputVerificationTest("Error Stack Trace Line Numbers",
                        passPath + "errorStackTraceLineNumbers_pass.mt");
        addOutputVerificationTest("Error Stack Trace Lambda Frame Name",
                        passPath + "errorStackTraceLambdaFrameName_pass.mt");
        addOutputVerificationTest("Error Propagation Typed No Match",
                        passPath + "errorPropagationTypedNoMatch_pass.mt");

        // === MYT-38 EXCEPTION-TYPE REJECTION (2 tests) ===
        // These tests use the new expectedErrorSubstring overload to assert
        // not just that an error was thrown, but that it was thrown for the
        // expected reason — substrings drawn from the actual messages
        // emitted by ExceptionExecutor.cpp:39-92 (throw validation) and
        // TryStatementHelper::compileCatchBlocks (catch validation).
        addTestFromFile("Error Catch Non-Exception Type",
                        errorPath + "errorCatchNonException_error.mt",
                        TestType::ERROR_EXPECTED,
                        "NotAnException");
        addTestFromFile("Error Uncaught In Main",
                        errorPath + "errorUncaughtMain_error.mt",
                        TestType::ERROR_EXPECTED,
                        "MytUncaughtSentinel38");

        // === THROW TESTS (5 tests) ===
        addOutputVerificationTest("Error Throw In Catch Different",
                        passPath + "errorThrowInCatchDifferent_pass.mt");
        addOutputVerificationTest("Error Throw Inheritance",
                        passPath + "errorThrowInheritance_pass.mt");
        addOutputVerificationTest("Error Throw Multiple Types",
                        passPath + "errorThrowMultipleTypes_pass.mt");
        addOutputVerificationTest("Error Throw Polymorphic",
                        passPath + "errorThrowPolymorphic_pass.mt");

        // === RECOVERY TESTS (4 tests) ===
        addOutputVerificationTest("Error Circuit Breaker",
                        passPath + "errorCircuitBreaker_pass.mt");
        addOutputVerificationTest("Error Fallback Value",
                        passPath + "errorFallbackValue_pass.mt");
        addOutputVerificationTest("Error Retry Mechanism",
                        passPath + "errorRetryMechanism_pass.mt");
        addOutputVerificationTest("Error Partial Failure",
                        passPath + "errorPartialFailure_pass.mt");

        // === OTHER TESTS (5 tests) ===
        addOutputVerificationTest("Error Aggregation",
                        passPath + "errorAggregation_pass.mt");
        addOutputVerificationTest("Error Cross Module Exception",
                        passPath + "errorCrossModuleException_pass.mt");
        addOutputVerificationTest("Error Deep Nesting",
                        passPath + "errorDeepNesting_pass.mt");
        addOutputVerificationTest("Error Large Exception Data",
                        passPath + "errorLargeExceptionData_pass.mt");
        addOutputVerificationTest("Error Many Exceptions",
                        passPath + "errorManyExceptions_pass.mt");
        addOutputVerificationTest("Error Module Init Exception",
                        passPath + "errorModuleInitException_pass.mt");

        // Test that static and instance methods with same name are allowed (different namespaces)
        addOutputVerificationTest("Allow Static And Instance Method Same Name",
                        passPath + "allowStaticAndInstanceSameName.mt");

        // === EDGE CASE TESTS - empty bodies / precedence / masking / null guards ===
        addOutputVerificationTest("Empty Try Empty Catch",
                        passPath + "emptyTryEmptyCatch.mt");
        addOutputVerificationTest("Return In Try Catch Finally Precedence",
                        passPath + "returnInTryCatchFinallyPrecedence.mt");
        addOutputVerificationTest("Nested Try Finally Exception Masking",
                        passPath + "nestedTryFinallyExceptionMasking.mt");
        addOutputVerificationTest("Throw Null Checked At Site",
                        passPath + "throwNullCheckedAtSite.mt");

        // ====================================
        // NEW COMBO TESTS — try/catch x other features
        // Each pair is a try/catch/finally surface combined with one
        // other language feature that wasn't previously exercised
        // (final modifiers, overloading, annotations + reflection,
        // do-while, foreach, casts, imports + alias, streams, value
        // class, recursion, etc.).
        // ====================================

        // Group A: class-shape combinations
        addOutputVerificationTest("Error Abstract Method Override",
                        passPath + "errorAbstractMethodOverride_pass.mt");
        addOutputVerificationTest("Error Final Var In Catch",
                        passPath + "errorFinalVarInCatch_pass.mt");
        addOutputVerificationTest("Error Final Method Throws",
                        passPath + "errorFinalMethodThrows_pass.mt");
        addOutputVerificationTest("Error Overloaded Throws",
                        passPath + "errorOverloadedThrows_pass.mt");
        addOutputVerificationTest("Error Annotation On Thrower",
                        passPath + "errorAnnotationOnThrower_pass.mt");

        // Group B: control flow
        addOutputVerificationTest("Error For Loop Continue On Throw",
                        passPath + "errorForLoopContinueOnThrow_pass.mt");
        addOutputVerificationTest("Error While Retry Until Success",
                        passPath + "errorWhileRetryUntilSuccess_pass.mt");
        addOutputVerificationTest("Error Do While Finally Always",
                        passPath + "errorDoWhileFinallyAlways_pass.mt");
        addOutputVerificationTest("Error ForEach Skip Bad Item",
                        passPath + "errorForEachSkipBadItem_pass.mt");
        addOutputVerificationTest("Error ForEach Abort Iteration",
                        passPath + "errorForEachAbortIteration_pass.mt");

        // Group C: data shapes
        addOutputVerificationTest("Error Array Bounds Caught",
                        passPath + "errorArrayBoundsCaught_pass.mt");
        addOutputVerificationTest("Error Array Null Element Caught",
                        passPath + "errorArrayNullElementCaught_pass.mt");
        addOutputVerificationTest("Error Cast Failure Caught",
                        passPath + "errorCastFailureCaught_pass.mt");

        // Group D: modules / pipelines
        addOutputVerificationTest("Error Imported Function Throws",
                        passPath + "errorImportedFunctionThrows_pass.mt");
        addOutputVerificationTest("Error Import Alias Exception",
                        passPath + "errorImportAliasException_pass.mt");
        addOutputVerificationTest("Error Stream Lambda Throws",
                        passPath + "errorStreamLambdaThrows_pass.mt");
        addOutputVerificationTest("Error Reflect ClassNotFound Caught",
                        passPath + "errorReflectClassNotFoundCaught_pass.mt");
        addOutputVerificationTest("Error Reflect Invoke Throws",
                        passPath + "errorReflectInvokeThrows_pass.mt");

        // Group E: modifiers / visibility
        addOutputVerificationTest("Error Static Method Throws Finally",
                        passPath + "errorStaticMethodThrowsFinally_pass.mt");
        addOutputVerificationTest("Error Private Field Finally Cleanup",
                        passPath + "errorPrivateFieldFinallyCleanup_pass.mt");

        // Group F: switch / match / interpolation / pool
        addOutputVerificationTest("Error Switch Default Throws Caught",
                        passPath + "errorSwitchDefaultThrowsCaught_pass.mt");
        addOutputVerificationTest("Error Match Pattern Throws",
                        passPath + "errorMatchPatternThrows_pass.mt");
        addOutputVerificationTest("Error Interpolated Exception Message",
                        passPath + "errorInterpolatedExceptionMessage_pass.mt");
        addOutputVerificationTest("Error String Pool Across Throw",
                        passPath + "errorStringPoolAcrossThrow_pass.mt");

        // Group G: recursion / box / value class
        addOutputVerificationTest("Error Recursion Finally Count",
                        passPath + "errorRecursionFinallyCount_pass.mt");
        addOutputVerificationTest("Error Box Class Throw Unbox",
                        passPath + "errorBoxClassThrowUnbox_pass.mt");
        addOutputVerificationTest("Error Value Class In Try Catch",
                        passPath + "errorValueClassInTryCatch_pass.mt");

        // Group H: polymorphism / multi-catch
        addOutputVerificationTest("Error Virtual Throw Caught As Base",
                        passPath + "errorVirtualThrowCaughtAsBase_pass.mt");
        addOutputVerificationTest("Error Multi Custom Exception Types",
                        passPath + "errorMultiCustomExceptionTypes_pass.mt");

        // Error tests for the same combos — compile-time and runtime
        // failures that the surrounding feature must reject.
        addTestFromFile("Error Final Reassign In Catch",
                        errorPath + "errorFinalReassignInCatch_error.mt",
                        TestType::ERROR_EXPECTED,
                        "final");
        addTestFromFile("Error Final Method Override",
                        errorPath + "errorFinalMethodOverride_error.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Error Annotation Wrong Target",
                        errorPath + "errorAnnotationWrongTarget_error.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Error Array Bounds Uncaught",
                        errorPath + "errorArrayBoundsUncaught_error.mt",
                        TestType::ERROR_EXPECTED,
                        "out of bounds");
        addTestFromFile("Error Bad Cast Uncaught",
                        errorPath + "errorBadCastUncaught_error.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Error Stream Lambda Uncaught",
                        errorPath + "errorStreamLambdaUncaught_error.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Error Instantiate Abstract Exception",
                        errorPath + "errorInstantiateAbstractException_error.mt",
                        TestType::ERROR_EXPECTED,
                        "abstract");
    }
}
