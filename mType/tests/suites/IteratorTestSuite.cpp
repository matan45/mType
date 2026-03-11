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

        addOutputVerificationTest("HashMap Key Iterator",
                                  passPath + "iteratorHashMapKeys.mt");

        addOutputVerificationTest("HashMap Entry Iterator",
                                  passPath + "iteratorHashMapEntries.mt");

        addOutputVerificationTest("HashMap Value Iterator",
                                  passPath + "iteratorHashMapValues.mt");

        // === ERROR TESTS ===
        addTestFromFile("HashMap Entry Iterator Past End",
                        errorPath + "hashMapEntryIteratorPastEnd.mt",
                        TestType::ERROR_EXPECTED);

        addTestFromFile("HashMap Value Iterator Past End",
                        errorPath + "hashMapValueIteratorPastEnd.mt",
                        TestType::ERROR_EXPECTED);

        addTestFromFile("HashMap Entry Iterator Empty Past End",
                        errorPath + "hashMapEntryIteratorEmptyPastEnd.mt",
                        TestType::ERROR_EXPECTED);

        addTestFromFile("HashMap Value Iterator Empty Past End",
                        errorPath + "hashMapValueIteratorEmptyPastEnd.mt",
                        TestType::ERROR_EXPECTED);
    }
}
