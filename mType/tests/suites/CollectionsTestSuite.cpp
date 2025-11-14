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
    }
}
