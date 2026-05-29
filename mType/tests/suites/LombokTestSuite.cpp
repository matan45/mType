#include "LombokTestSuite.hpp"

namespace tests::testSuite
{
    using namespace testFramework;

    void LombokTestSuite::setupTests()
    {
        // ===== PASS TESTS =====

        addOutputVerificationTest("Getter + AllArgsConstructor",
                                  passPath + "getter_allargs_pass.mt");

        addOutputVerificationTest("Setter + NoArgsConstructor",
                                  passPath + "setter_pass.mt");

        addOutputVerificationTest("ToString",
                                  passPath + "tostring_pass.mt");

        addOutputVerificationTest("Data (getters/toString + structural equals/hashCode)",
                                  passPath + "data_pass.mt");

        addOutputVerificationTest("Builder fluent chain",
                                  passPath + "builder_pass.mt");

        addOutputVerificationTest("AllArgsConstructor inheritance (super forwarding)",
                                  passPath + "allargs_inheritance_pass.mt");

        addOutputVerificationTest("Object-typed getters + nested toString",
                                  passPath + "object_fields_pass.mt");

        addOutputVerificationTest("Data with only non-final fields (no-arg ctor)",
                                  passPath + "noargs_data_combo_pass.mt");

        addOutputVerificationTest("Skip-if-exists: user getter not overwritten",
                                  passPath + "skip_existing_getter_pass.mt");

        // ===== ERROR TESTS =====

        addTestFromFile("@Getter on function rejected (@Target)",
                        errorPath + "getter_on_function_error.mt",
                        TestType::ERROR_EXPECTED,
                        "cannot be applied to");

        addTestFromFile("@Data on function rejected (@Target)",
                        errorPath + "data_on_function_error.mt",
                        TestType::ERROR_EXPECTED,
                        "cannot be applied to");

        addTestFromFile("Generic class skipped — getter not generated",
                        errorPath + "generic_class_skip_error.mt",
                        TestType::ERROR_EXPECTED);
    }
}
