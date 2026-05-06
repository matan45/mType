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

        // === EDGE CASE COVERAGE ===
        addOutputVerificationTest("Stream FlatMap Mixed Inner",
                                  passPath + "streamFlatMap.mt");
        addOutputVerificationTest("Stream FlatMap Empty Outer",
                                  passPath + "streamFlatMapEmptyOuter.mt");
        addOutputVerificationTest("Stream Peek Chained After Filter",
                                  passPath + "streamPeekChained.mt");
        addOutputVerificationTest("Stream Peek vs ForEach Order",
                                  passPath + "streamPeekVsForEachOrder.mt");
        addOutputVerificationTest("Stream Peek Without Terminal",
                                  passPath + "streamPeekNotConsumed.mt");
        addOutputVerificationTest("Stream Min/Max Basic",
                                  passPath + "streamMinMaxBasic.mt");
        addOutputVerificationTest("Stream Min/Max Ties",
                                  passPath + "streamMinMaxTies.mt");
        addOutputVerificationTest("Stream Reduce Without Identity",
                                  passPath + "streamReduceNoIdentity.mt");
        addOutputVerificationTest("Stream Reduce Single Element",
                                  passPath + "streamReduceNoIdentitySingleElement.mt");
        addOutputVerificationTest("Stream NoneMatch",
                                  passPath + "streamNoneMatch.mt");
        addOutputVerificationTest("Stream From LinkedList",
                                  passPath + "streamLinkedListSource.mt");
        addOutputVerificationTest("Stream Empty Terminals",
                                  passPath + "streamEmptyTerminals.mt");

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

        // === EDGE CASE ERROR TESTS ===
        addTestFromFile("Stream Filter Predicate Throws",
                       errorPath + "streamFilterPredicateThrows.mt",
                       TestType::ERROR_EXPECTED);

        addTestFromFile("Stream Map Mapper Throws",
                       errorPath + "streamMapMapperThrows.mt",
                       TestType::ERROR_EXPECTED);

        addTestFromFile("Stream Reduce Accumulator Throws",
                       errorPath + "streamReduceMapperThrows.mt",
                       TestType::ERROR_EXPECTED);

        addTestFromFile("Stream Min On Empty",
                       errorPath + "streamMinEmpty.mt",
                       TestType::ERROR_EXPECTED);
    }
}
