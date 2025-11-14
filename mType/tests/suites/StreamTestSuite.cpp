#include "StreamTestSuite.hpp"

namespace tests::testSuite
{
    using namespace testFramework;

    void StreamTestSuite::setupTests()
    {
        // Add pass tests for Stream API functional operations
        addOutputVerificationTest("Stream Basic Operations (filter, map)",
                                  passPath + "streamBasicOperations.mt");

        addOutputVerificationTest("Stream Count Operation",
                                  passPath + "streamCount.mt");

        addOutputVerificationTest("Stream ForEach Operation",
                                  passPath + "streamForEach.mt");

        addOutputVerificationTest("Stream Reduce Operation",
                                  passPath + "streamReduce.mt");

        addOutputVerificationTest("Stream Limit Operation",
                                  passPath + "streamLimit.mt");

        addOutputVerificationTest("Stream Skip Operation",
                                  passPath + "streamSkip.mt");

        addOutputVerificationTest("Stream Distinct Operation",
                                  passPath + "streamDistinct.mt");

        addOutputVerificationTest("Stream AnyMatch Operation",
                                  passPath + "streamAnyMatch.mt");

        addOutputVerificationTest("Stream AllMatch Operation",
                                  passPath + "streamAllMatch.mt");

        addOutputVerificationTest("Stream Operation Chaining",
                                  passPath + "streamChaining.mt");

        addOutputVerificationTest("Stream HashMap Operations",
                                  passPath + "streamHashMap.mt");

        addOutputVerificationTest("Stream HashMap Advanced Operations",
                                  passPath + "streamHashMapAdvanced.mt");

        addOutputVerificationTest("Stream SortedWith Ascending",
                                  passPath + "streamSortedWith.mt");

        addOutputVerificationTest("Stream SortedWith Descending",
                                  passPath + "streamSortedWithDescending.mt");

        addOutputVerificationTest("Stream SortedWith Chaining",
                                  passPath + "streamSortedWithChaining.mt");
    }
}
