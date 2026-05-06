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

        addOutputVerificationTest("ForEach Over int[][] Matrix",
                                  passPath + "forEachIntArrayMatrix.mt");

        addOutputVerificationTest("ForEach With Lambda In Body",
                                  passPath + "forEachWithLambdaInBody.mt");

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
    }
}
