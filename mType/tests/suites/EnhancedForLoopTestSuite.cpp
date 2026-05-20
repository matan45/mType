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
        addOutputVerificationTest("ForEach Cast Result",
                                  passPath + "forEachCastResult.mt");
        // ForEach Ternary Result was moved to the CANARY section below — it
        // hits the same Object-typed-iterable bug as the nested-generic
        // canaries (the bug surface is wider than originally documented).

        // === CONCURRENT MODIFICATION ===
        addOutputVerificationTest("ForEach Read Field Mutated In Body",
                                  passPath + "forEachReadFieldMutatedInBody.mt");
        // REMOVED - forEachMutateUnderlyingList (structural mutation of the
        // iterated collection does not throw in mType; whether that's by
        // design or a missing safety check is undetermined. If we decide
        // it's a bug, file a ticket and restore as an ERROR_EXPECTED canary).
        // REMOVED - forEachNonNullableOverNullableSource (the construct
        // depended on `Box?[]` declaration syntax, which the parser rejects;
        // there's no other obvious way to express a nullable-element source
        // without invoking the same unsupported syntax).

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

        // === CANARIES (MYT-350: FOR-EACH LOSES ELEMENT TYPE) ===
        // These are kept failing on purpose until MYT-350 lands. Root cause:
        // for-each rejects with MT-E2007 whenever the source expression's
        // static type collapses to Object — declared loop-variable type isn't
        // used as a binding hint. Surface: nested generic collections, 2D
        // primitive arrays, and ternary-result iterables. See
        // memory:project_foreach_loses_nested_generic_type and
        // memory:feedback_keep_failing_canary_tests.
        addOutputVerificationTest("CANARY ForEach Nested Generic Collection",
                                  passPath + "forEachNestedGenericCollection.mt");

        addOutputVerificationTest("CANARY ForEach Nested int[][] 2D",
                                  passPath + "forEachNestedIntArray2D.mt");

        // CANARY (MYT-350, wider surface): for-each over a ternary expression
        // also loses element type. Same root cause as the two nested canaries
        // above — confirmed during first build pass. Documents that the bug
        // hits any Object-typed iterable source, not just nested collections.
        addOutputVerificationTest("CANARY ForEach Ternary Result",
                                  passPath + "forEachTernaryResult.mt");

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
        addTestFromFile("For-Each Final Loop Var Rejects Reassignment",
                       errorPath + "forEachFinalLoopVarReassign.mt",
                       TestType::ERROR_EXPECTED);
    }
}
