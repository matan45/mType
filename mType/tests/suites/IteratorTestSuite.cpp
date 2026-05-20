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

        // === EDGE CASE COVERAGE ===
        addOutputVerificationTest("Iterator Repeated HasNext on Empty",
                                  passPath + "iteratorRepeatedHasNextEmpty.mt");

        addOutputVerificationTest("Iterator Repeated HasNext After Exhaustion",
                                  passPath + "iteratorRepeatedHasNextAfterExhaustion.mt");

        addOutputVerificationTest("Iterator Close Without Advance",
                                  passPath + "iteratorNeverAdvancedClose.mt");

        addOutputVerificationTest("Stack Iterator Basic",
                                  passPath + "iteratorStack.mt");

        addOutputVerificationTest("Empty Stack Iterator",
                                  passPath + "iteratorStackEmpty.mt");

        addOutputVerificationTest("ArrayQueue Iterator Basic",
                                  passPath + "iteratorArrayQueue.mt");

        addOutputVerificationTest("ArrayQueue Iterator After Wrap-Around",
                                  passPath + "iteratorArrayQueueAfterWrapAround.mt");

        addOutputVerificationTest("Empty HashSet Iterator",
                                  passPath + "iteratorHashSetEmpty.mt");

        addOutputVerificationTest("Iterator Close Idempotent",
                                  passPath + "iteratorCloseIdempotent.mt");

        addOutputVerificationTest("Iterator After Clear",
                                  passPath + "iteratorAfterClear.mt");

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

        addTestFromFile("ArrayList Iterator Past End",
                        errorPath + "arrayListIteratorPastEnd.mt",
                        TestType::ERROR_EXPECTED);

        addTestFromFile("LinkedList Iterator Past End",
                        errorPath + "linkedListIteratorPastEnd.mt",
                        TestType::ERROR_EXPECTED);

        addTestFromFile("Iterator Empty Next Throws",
                        errorPath + "iteratorEmptyNextThrows.mt",
                        TestType::ERROR_EXPECTED);

        addTestFromFile("Stack Iterator Past End",
                        errorPath + "stackIteratorPastEnd.mt",
                        TestType::ERROR_EXPECTED);

        addTestFromFile("ArrayQueue Iterator Past End",
                        errorPath + "arrayQueueIteratorPastEnd.mt",
                        TestType::ERROR_EXPECTED);

        addTestFromFile("HashSet Iterator Past End",
                        errorPath + "hashSetIteratorPastEnd.mt",
                        TestType::ERROR_EXPECTED);
    }
}
