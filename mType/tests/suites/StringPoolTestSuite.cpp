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
    }
}
