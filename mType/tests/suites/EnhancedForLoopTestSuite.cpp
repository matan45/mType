#include "EnhancedForLoopTestSuite.hpp"

namespace tests::testSuite
{
    using namespace testFramework;

    void EnhancedForLoopTestSuite::setupTests()
    {
        // Add pass tests for enhanced for-loop syntax: for(T x : collection)
        addOutputVerificationTest("Enhanced For-Loop with ArrayList",
                                  passPath + "forEachArrayList.mt");

        addOutputVerificationTest("Enhanced For-Loop with HashSet",
                                  passPath + "forEachHashSet.mt");

        addOutputVerificationTest("Enhanced For-Loop with HashMap",
                                  passPath + "forEachHashMap.mt");

        addOutputVerificationTest("Enhanced For-Loop with LinkedList",
                                  passPath + "forEachLinkedList.mt");

        addOutputVerificationTest("Enhanced For-Loop with Queue",
                                  passPath + "forEachQueue.mt");

        addOutputVerificationTest("Enhanced For-Loop with Stack",
                                  passPath + "forEachStack.mt");

        addOutputVerificationTest("Nested Enhanced For-Loops",
                                  passPath + "forEachNested.mt");

        addOutputVerificationTest("Enhanced For-Loop with Empty Collection",
                                  passPath + "forEachEmpty.mt");

        addOutputVerificationTest("Enhanced For-Loop with Generic Class Context",
                                  passPath + "forEachGenericClass.mt");

        // === CROSS-FEATURE EDGE CASES ===
        addOutputVerificationTest("ForEach TryCatch Generic Collection",
                                  passPath + "forEachTryCatchGenericCollection.mt");

        // === CONTROL-FLOW & RESOURCE-CLEANUP EDGE CASES ===
        addOutputVerificationTest("ForEach Break",
                                  passPath + "forEachBreak.mt");

        addOutputVerificationTest("ForEach Continue",
                                  passPath + "forEachContinue.mt");

        addOutputVerificationTest("ForEach Return Early From Function",
                                  passPath + "forEachReturnEarly.mt");

        addOutputVerificationTest("ForEach Nested Same Collection",
                                  passPath + "forEachNestedSameCollection.mt");

        addOutputVerificationTest("ForEach Over Stream Result Array",
                                  passPath + "forEachOverStreamResultArray.mt");

        addOutputVerificationTest("ForEach Throw In Body Then Re-Iterate",
                                  passPath + "forEachThrowInBodyClose.mt");

        addOutputVerificationTest("ForEach LinkedList Break/Continue",
                                  passPath + "forEachLinkedListBreakContinue.mt");

        // === ADDITIONAL EDGE-CASE COVERAGE ===
        addOutputVerificationTest("ForEach Single Element",
                                  passPath + "forEachSingleElement.mt");

        addOutputVerificationTest("ForEach Empty Body",
                                  passPath + "forEachEmptyBody.mt");

        addOutputVerificationTest("ForEach Back-To-Back Same Collection",
                                  passPath + "forEachBackToBackSameCollection.mt");

        addOutputVerificationTest("ForEach Three-Level Nested With Inner Break",
                                  passPath + "forEachThreeNestedBreakInner.mt");

        addOutputVerificationTest("ForEach Over Method Call Result",
                                  passPath + "forEachOverMethodCallResult.mt");

        addOutputVerificationTest("ForEach Over This.Field",
                                  passPath + "forEachOverThisField.mt");

        addOutputVerificationTest("ForEach Loop Var Shadows Outer",
                                  passPath + "forEachShadowOuter.mt");

        addOutputVerificationTest("ForEach With Lambda In Body",
                                  passPath + "forEachWithLambdaInBody.mt");

        // === ARRAY ITERATION ===
        addOutputVerificationTest("ForEach Over int[] Primitive Array",
                                  passPath + "forEachIntArray.mt");

        addOutputVerificationTest("ForEach Over float[] Primitive Array",
                                  passPath + "forEachFloatArray.mt");

        addOutputVerificationTest("ForEach Over bool[] Primitive Array",
                                  passPath + "forEachBoolArray.mt");

        addOutputVerificationTest("ForEach Over String[] Class Array",
                                  passPath + "forEachStringArray.mt");

        addOutputVerificationTest("ForEach Over User Class Array",
                                  passPath + "forEachClassArray.mt");

        // === POLYMORPHIC / INHERITANCE ITERATION ===
        addOutputVerificationTest("ForEach Polymorphic Elements",
                                  passPath + "forEachPolymorphicElements.mt");

        addOutputVerificationTest("ForEach Interface-Typed Loop Var",
                                  passPath + "forEachInterfaceTypedLoopVar.mt");

        // === ITERATION OVER EXPRESSION RESULTS ===
        addOutputVerificationTest("ForEach Ternary Result",
                                  passPath + "forEachTernaryResult.mt");

        addOutputVerificationTest("ForEach Cast Result",
                                  passPath + "forEachCastResult.mt");

        // === CONCURRENT MODIFICATION ===
        addOutputVerificationTest("ForEach Read Field Mutated In Body",
                                  passPath + "forEachReadFieldMutatedInBody.mt");

        // === HOT-LOOP / JIT VARIANTS ===
        addOutputVerificationTest("ForEach int[] HotLoop",
                                  passPath + "forEachIntArrayHotLoop.mt");

        addOutputVerificationTest("ForEach ArrayList<Int> HotLoop",
                                  passPath + "forEachArrayListHotLoop.mt");

        // === NULLABLE ELEMENT TYPES ===
        addOutputVerificationTest("ForEach Nullable Element Type",
                                  passPath + "forEachNullableElementType.mt");

        // === LOOP-VARIABLE SEMANTICS ===
        addOutputVerificationTest("ForEach Loop Var Reassign Allowed",
                                  passPath + "forEachLoopVarReassign.mt");

        // === CANARIES (MYT-NEW: NESTED GENERIC FOREACH) ===
        // These are kept failing on purpose until MYT-NEW lands. The bug:
        // outer for-each over a nested-generic collection binds the row
        // correctly, but the inner for-each then types the element as Object
        // and rejects with MT-E2007. See
        // memory:project_foreach_loses_nested_generic_type and
        // memory:feedback_keep_failing_canary_tests.
        addOutputVerificationTest("CANARY ForEach Nested Generic Collection",
                                  passPath + "forEachNestedGenericCollection.mt");

        addOutputVerificationTest("CANARY ForEach Nested int[][] 2D",
                                  passPath + "forEachNestedIntArray2D.mt");

        // Add error tests for iterator type safety
        addTestFromFile("For-Each on Non-Iterable Type",
                       errorPath + "forEachNonIterable.mt",
                       TestType::ERROR_EXPECTED);

        addTestFromFile("For-Each with Type Mismatch",
                       errorPath + "forEachTypeMismatch.mt",
                       TestType::ERROR_EXPECTED);

        addTestFromFile("For-Each with Primitive Type Mismatch",
                       errorPath + "forEachPrimitiveMismatch.mt",
                       TestType::ERROR_EXPECTED);

        addTestFromFile("For-Each on Null Collection",
                       errorPath + "forEachNullCollection.mt",
                       TestType::ERROR_EXPECTED);

        // === ADDED ERROR PATHS ===
        // Pin with expectedErrorSubstring after first build captures actual
        // what() strings.
        addTestFromFile("For-Each Structural Mutation Of Source",
                       errorPath + "forEachMutateUnderlyingList.mt",
                       TestType::ERROR_EXPECTED);

        addTestFromFile("For-Each Non-Nullable Var Over Nullable Source",
                       errorPath + "forEachNonNullableOverNullableSource.mt",
                       TestType::ERROR_EXPECTED);

        addTestFromFile("For-Each Final Loop Var Rejects Reassignment",
                       errorPath + "forEachFinalLoopVarReassign.mt",
                       TestType::ERROR_EXPECTED);
    }
}
