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

        // === CROSS-FEATURE EDGE CASES ===
        addOutputVerificationTest("Stream Empty Array",
                                  passPath + "streamEmptyArray.mt");
        addOutputVerificationTest("Stream Box Conversions",
                                  passPath + "streamBoxConversions.mt");

        // Add error tests for stream error handling
        addTestFromFile("Stream Reuse After Terminal Operation",
                       errorPath + "streamReuse.mt",
                       TestType::ERROR_EXPECTED);

        addTestFromFile("Stream Reuse After Intermediate Operation",
                       errorPath + "streamReuseIntermediate.mt",
                       TestType::ERROR_EXPECTED);

        addTestFromFile("Stream Null Predicate",
                       errorPath + "streamNullPredicate.mt",
                       TestType::ERROR_EXPECTED);

        addTestFromFile("Stream Null Mapper Function",
                       errorPath + "streamNullMapper.mt",
                       TestType::ERROR_EXPECTED);

        addTestFromFile("Stream Null Consumer",
                       errorPath + "streamNullConsumer.mt",
                       TestType::ERROR_EXPECTED);

        addTestFromFile("Stream Limit with Negative Value",
                       errorPath + "streamLimitNegative.mt",
                       TestType::ERROR_EXPECTED);

        addTestFromFile("Stream Skip with Negative Value",
                       errorPath + "streamSkipNegative.mt",
                       TestType::ERROR_EXPECTED);
    }
}
