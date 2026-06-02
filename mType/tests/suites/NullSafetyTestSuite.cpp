#include "NullSafetyTestSuite.hpp"

namespace tests::testSuite
{
    using namespace testFramework;

    void NullSafetyTestSuite::setupTests()
    {
        // === NULLABLE TYPE BASICS ===
        addOutputVerificationTest("Nullable Type Basic",
                        passPath + "nullableTypeBasic.mt");

        // === SMART CAST (NULL NARROWING) ===
        addOutputVerificationTest("Smart Cast Basic",
                        passPath + "smartCastBasic.mt");

        // === NULLABLE FUNCTION PARAMS & RETURN TYPES ===
        addOutputVerificationTest("Nullable Function Params",
                        passPath + "nullableFunctionParams.mt");

        // === NULLABLE VALUE OBJECTS ===
        addOutputVerificationTest("Nullable Value Object",
                        passPath + "nullableValueObject.mt");

        // === JIT NULL CHECK ELIMINATION ===
        addOutputVerificationTest("JIT Null Check Elimination",
                        passPath + "jitNullCheckElimination.mt");

        // === NULL SAFE PARAM AND RETURN ===
        addOutputVerificationTest("Null Safe Param and Return",
                        passPath + "nullSafeParamAndReturn.mt");

        // === GUARD CLAUSE NARROWING ===
        addOutputVerificationTest("Guard Clause Narrowing",
                        passPath + "guardClauseNarrowing.mt");
        addOutputVerificationTest("Short-Circuit && Null Guard",
                        passPath + "shortCircuitAndNullGuard.mt");
        addOutputVerificationTest("Short-Circuit || Null Guard",
                        passPath + "shortCircuitOrNullGuard.mt");
        addOutputVerificationTest("Compound If Condition Narrowing",
                        passPath + "compoundIfConditionNarrowing.mt");
        addOutputVerificationTest("Compound Guard Clause Narrowing",
                        passPath + "compoundGuardClauseNarrowing.mt");

        // === ERROR TESTS ===
        addTestFromFile("Null to Non-Nullable Assignment",
                        errorPath + "nullToNonNullable_error.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Nullable Primitive Type",
                        errorPath + "nullablePrimitiveType_error.mt",
                        TestType::ERROR_EXPECTED);

        // === COMPILE-TIME NULL SAFETY ENFORCEMENT ===
        addTestFromFile("Member Access on Nullable Receiver",
                        errorPath + "memberAccessOnNullable_error.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Method Call on Nullable Receiver",
                        errorPath + "methodCallOnNullable_error.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Member Assignment on Nullable Receiver",
                        errorPath + "memberAssignOnNullable_error.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Chained Access Through Nullable Field",
                        errorPath + "chainedAccessThroughNullable_error.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Null Arg to Non-Null Parameter",
                        errorPath + "nullArgToNonNullParam_error.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Nullable Arg to Non-Null Parameter",
                        errorPath + "nullableArgToNonNullParam_error.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Null Arg to Non-Null Constructor Parameter",
                        errorPath + "nullArgToNonNullCtorParam_error.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Return Null from Non-Null Function",
                        errorPath + "returnNullFromNonNull_error.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Nullable Parameter Not Narrowed",
                        errorPath + "nullableParamNotNarrowed_error.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Guard Clause Without Exit Not Narrowed",
                        errorPath + "guardClauseNoExit_error.mt",
                        TestType::ERROR_EXPECTED);

        // === NARROWING EDGE CASES ===
        addOutputVerificationTest("Else-Branch Method Dispatch",
                        passPath + "elseBranchMethodDispatch.mt");
        addOutputVerificationTest("Throw Guard Narrowing",
                        passPath + "throwGuardNarrowing.mt");
        addOutputVerificationTest("While Loop Narrowing",
                        passPath + "whileLoopNarrowing.mt");
        addOutputVerificationTest("Nested Narrowing",
                        passPath + "nestedNarrowing.mt");
        addTestFromFile("Narrowing Scoped to If Block",
                        errorPath + "narrowingScopedToIfBlock_error.mt",
                        TestType::ERROR_EXPECTED);

        // === FIELDS AND CHAINED ACCESS ===
        addOutputVerificationTest("Nullable Field Assign and Get",
                        passPath + "nullableFieldAssignAndGet.mt");
        addOutputVerificationTest("Local Copy of Field Narrowed",
                        passPath + "localCopyOfFieldNarrowed.mt");
        addOutputVerificationTest("Nullable Field Defaults to Null",
                        passPath + "nullableFieldDefaultsToNull.mt");
        // === ARRAYS WITH NULLABLE ELEMENTS ===
        addOutputVerificationTest("Object Array Defaults Null",
                        passPath + "objectArrayDefaultsNull.mt");
        addOutputVerificationTest("Array Iterate With Null Check",
                        passPath + "arrayIterateWithNullCheck.mt");
        addOutputVerificationTest("Local Copy From Array Narrowed",
                        passPath + "localCopyFromArrayNarrowed.mt");
        addOutputVerificationTest("Array Element Null Then Reassign",
                        passPath + "arrayElementNullThenReassign.mt");

        // === INTERFACES, INHERITANCE, OBJECT ===
        addOutputVerificationTest("Nullable Interface Receiver",
                        passPath + "nullableInterfaceReceiver.mt");
        addOutputVerificationTest("Nullable Inherited Receiver",
                        passPath + "nullableInheritedReceiver.mt");
        addOutputVerificationTest("Nullable Object Receiver",
                        passPath + "nullableObjectReceiver.mt");
        addTestFromFile("Nullable Interface Unchecked",
                        errorPath + "nullableInterfaceUnchecked_error.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Nullable Super Unchecked",
                        errorPath + "nullableSuperUnchecked_error.mt",
                        TestType::ERROR_EXPECTED);

        // === SAFE NAVIGATION OPERATOR (?.) — MYT-374 ===
        addOutputVerificationTest("Safe Nav Field Access",
                        passPath + "safeNavFieldAccess.mt");
        addOutputVerificationTest("Safe Nav Method With Args",
                        passPath + "safeNavMethodWithArgs.mt");
        addOutputVerificationTest("Safe Nav Generic Receiver",
                        passPath + "safeNavGenericReceiver.mt");
        addOutputVerificationTest("Safe Nav Polymorphic Dispatch",
                        passPath + "safeNavPolymorphicDispatch.mt");
        addOutputVerificationTest("Safe Nav Inherited / Super Delegation",
                        passPath + "safeNavInheritedSuper.mt");
        // MYT-374 review finding 1: ?. must short-circuit on array element access.
        addOutputVerificationTest("Safe Nav Array Element Field",
                        passPath + "safeNavArrayElementField.mt");
        addOutputVerificationTest("Safe Nav In Return Position",
                        passPath + "safeNavReturn.mt");
        // MYT-374 review finding 2: safe-navigation assignment target.
        addOutputVerificationTest("Safe Nav Assignment",
                        passPath + "safeNavAssign.mt");
        addOutputVerificationTest("Safe Nav Index-Target Assignment",
                        passPath + "safeNavIndexAssign.mt");
        addTestFromFile("Safe Nav Compound Assignment Rejected",
                        errorPath + "safeNavCompoundAssign_error.mt",
                        TestType::ERROR_EXPECTED,
                        "compound-assignment");
        // Safe-nav result is nullable: feeding it to a non-null parameter, or
        // continuing the chain with a plain '.', must be rejected.
        addTestFromFile("Safe Nav Result To Non-Null Param",
                        errorPath + "safeNavResultToNonNullParam_error.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Safe Nav Unsafe Continuation",
                        errorPath + "safeNavUnsafeContinuation_error.mt",
                        TestType::ERROR_EXPECTED,
                        "nullable receiver");
    }
}
