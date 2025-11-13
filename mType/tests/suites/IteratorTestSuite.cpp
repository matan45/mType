#include "IteratorTestSuite.hpp"

namespace tests::testSuite
{
    using namespace testFramework;

    void IteratorTestSuite::setupTests()
    {
        // Add pass tests for iterator functionality
        addOutputVerificationTest("Basic ArrayList Iterator",
                                  passPath + "iteratorBasic.mt");

        addOutputVerificationTest("HashSet Iterator",
                                  passPath + "iteratorHashSet.mt");

        addOutputVerificationTest("LinkedList Iterator",
                                  passPath + "iteratorLinkedList.mt");

        addOutputVerificationTest("Empty Collection Iterator",
                                  passPath + "iteratorEmpty.mt");
    }
}
