#include "StringPoolTestSuite.hpp"

namespace tests::testSuite
{
    using namespace testFramework;

    void StringPoolTestSuite::setupTests()
    {
        // Core string pool operations
        addOutputVerificationTest("Basic String Pool Functionality",
                                  stringPoolPath + "basic_string_pool_test.mt");

        addOutputVerificationTest("String Pool Performance Test",
                                  stringPoolPath + "string_pool_performance_test.mt");

        // Integration with classes and objects
        addOutputVerificationTest("String Pool Class Integration",
                                  stringPoolPath + "string_pool_integration_test.mt");


        // Master comprehensive test with detailed validation
        addOutputVerificationTest("String Pool Comprehensive Test",
                                  stringPoolPath + "string_pool_comprehensive_test.mt");


        // Special character handling
        addOutputVerificationTest("String Pool Edge Cases",
                                  stringPoolPath + "string_pool_stress_test.mt");

        // Pool effectiveness measurement
        addOutputVerificationTest("String Pool Effectiveness",
                                  stringPoolPath + "string_pool_efficiency_test.mt");

        // Pool effectiveness measurement
        addOutputVerificationTest("String Pool Single character Stress Test",
                                  stringPoolPath + "debug_single_chars.mt");

        // Parameter string comparison (entityName == "literal" pattern)
        addOutputVerificationTest("String Compare Parameter Pattern",
                                  stringPoolPath + "string_compare_parameter_test.mt");

        // === EDGE CASES ===
        addOutputVerificationTest("StringPool Empty String Identity",
                                  stringPoolPath + "string_pool_empty_string.mt");

        addOutputVerificationTest("StringPool Concat Identity",
                                  stringPoolPath + "string_pool_concat_identity.mt");

        addOutputVerificationTest("StringPool Substring Pooled",
                                  stringPoolPath + "string_pool_substring_pooled.mt");

        addOutputVerificationTest("StringPool Case Conversion",
                                  stringPoolPath + "string_pool_case_conversion.mt");

        addOutputVerificationTest("StringPool Long String",
                                  stringPoolPath + "string_pool_long_string.mt");

        addOutputVerificationTest("StringPool Unicode",
                                  stringPoolPath + "string_pool_unicode.mt");

        addOutputVerificationTest("StringPool Across Scope",
                                  stringPoolPath + "string_pool_across_scope.mt");

        addOutputVerificationTest("StringPool String Wrapper Identity",
                                  stringPoolPath + "string_pool_wrapper_identity.mt");

        // === ERROR TESTS ===
        addTestFromFile("Substring Out Of Range",
                        errorPath + "string_substring_out_of_range_error.mt",
                        TestType::ERROR_EXPECTED,
                        "out of bounds");

        addTestFromFile("Substring Negative Start",
                        errorPath + "string_substring_negative_start_error.mt",
                        TestType::ERROR_EXPECTED,
                        "cannot be negative");

        addTestFromFile("String Relational GE Rejected",
                        errorPath + "string_compare_ge_error.mt",
                        TestType::ERROR_EXPECTED,
                        "GE requires numeric operands");
    }
}
