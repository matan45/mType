#include "CollectionsTestSuite.hpp"

namespace tests::testSuite
{
    using namespace testFramework;

    void CollectionsTestSuite::setupTests()
    {
        // ArrayList sorting tests
        addOutputVerificationTest("ArrayList Sort Ascending",
                                  passPath + "arrayListSort.mt");

        addOutputVerificationTest("ArrayList Sort Descending",
                                  passPath + "arrayListSortDescending.mt");

        addOutputVerificationTest("ArrayList Sort Strings by Length",
                                  passPath + "arrayListSortStrings.mt");

        // LinkedList sorting tests
        addOutputVerificationTest("LinkedList Sort Ascending",
                                  passPath + "linkedListSort.mt");

        addOutputVerificationTest("LinkedList Sort Descending",
                                  passPath + "linkedListSortDescending.mt");

        // === HASHMAP / HASHSET COLLISION & RESIZE EDGE CASES ===
        addOutputVerificationTest("HashMap Collision Remove BackShift",
                                  passPath + "hashMapCollisionRemoveBackShift.mt");

        addOutputVerificationTest("HashMap Collision Remove Middle",
                                  passPath + "hashMapCollisionRemoveMiddle.mt");

        addOutputVerificationTest("HashSet Collision Remove BackShift",
                                  passPath + "hashSetCollisionRemoveBackShift.mt");

        addOutputVerificationTest("HashMap Remove Then Put Same Key",
                                  passPath + "hashMapRemoveThenPutSameKey.mt");

        addOutputVerificationTest("HashMap Put Overwrite Same Key",
                                  passPath + "hashMapPutOverwriteSameKey.mt");

        addOutputVerificationTest("HashMap Wrap-Around Collision",
                                  passPath + "hashMapWrapAroundCollision.mt");

        addOutputVerificationTest("HashMap Resize Keeps All Keys",
                                  passPath + "hashMapResizeKeepsAllKeys.mt");

        addOutputVerificationTest("HashSet Resize Keeps All Elements",
                                  passPath + "hashSetResizeKeepsAllElements.mt");

        addOutputVerificationTest("HashMap Clear Then Reuse",
                                  passPath + "hashMapClearThenReuse.mt");

        // === QUEUE / STACK / ARRAYLIST EDGE CASES ===
        addOutputVerificationTest("ArrayQueue Wrap-Around",
                                  passPath + "arrayQueueWrapAround.mt");

        addOutputVerificationTest("ArrayQueue Resize Preserves Order",
                                  passPath + "arrayQueueResizePreservesOrder.mt");

        addOutputVerificationTest("Stack Mixed Push/Pop",
                                  passPath + "stackMixedPushPop.mt");

        addOutputVerificationTest("ArrayList Resize Many Adds",
                                  passPath + "arrayListResizeManyAdds.mt");

        addOutputVerificationTest("ArrayList Nested Resize During Substring Scan",
                                  passPath + "arrayListNestedResizeDuringSubstringScan.mt");

        // === LINKEDLIST REMOVE EDGE CASES ===
        addOutputVerificationTest("LinkedList Remove Middle",
                                  passPath + "linkedListRemoveMiddle.mt");

        addOutputVerificationTest("LinkedList Remove Head And Tail",
                                  passPath + "linkedListRemoveHeadAndTail.mt");

        addOutputVerificationTest("LinkedList Remove Single Element",
                                  passPath + "linkedListRemoveSingleElement.mt");

        // === ERROR TESTS ===
        addTestFromFile("ArrayList Get Out of Bounds",
                        errorPath + "arrayListGetOutOfBounds.mt",
                        TestType::ERROR_EXPECTED);

        addTestFromFile("ArrayList Set Out of Bounds",
                        errorPath + "arrayListSetOutOfBounds.mt",
                        TestType::ERROR_EXPECTED);

        addTestFromFile("ArrayList Get Negative Index",
                        errorPath + "arrayListGetNegativeIndex.mt",
                        TestType::ERROR_EXPECTED);

        addTestFromFile("ArrayList Set Negative Index",
                        errorPath + "arrayListSetNegativeIndex.mt",
                        TestType::ERROR_EXPECTED);

        addTestFromFile("ArrayList Get Empty List",
                        errorPath + "arrayListGetEmptyList.mt",
                        TestType::ERROR_EXPECTED);

        addTestFromFile("HashMap Get Missing After Collision Remove",
                        errorPath + "hashMapGetMissingAfterCollisionRemove.mt",
                        TestType::ERROR_EXPECTED);
    }
}
