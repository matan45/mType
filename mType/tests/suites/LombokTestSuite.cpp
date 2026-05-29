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

        addOutputVerificationTest("Getter + Setter only (default ctor still auto-generated)",
                                  passPath + "getter_setter_only_pass.mt");

        addOutputVerificationTest("ToString",
                                  passPath + "tostring_pass.mt");

        addOutputVerificationTest("Data (getters/toString + structural equals/hashCode)",
                                  passPath + "data_pass.mt");

        addOutputVerificationTest("Builder fluent chain",
                                  passPath + "builder_pass.mt");

        addOutputVerificationTest("Builder setters are order-independent (int/string/bool)",
                                  passPath + "builder_reorder_pass.mt");

        addOutputVerificationTest("Builder over inheritance (mirrors inherited + own fields)",
                                  passPath + "builder_inheritance_pass.mt");

        addOutputVerificationTest("Builder reuse — independent instances",
                                  passPath + "builder_reuse_pass.mt");

        addOutputVerificationTest("Builder reuses a user-defined constructor (no duplicate)",
                                  passPath + "builder_explicit_ctor_pass.mt");

        addOutputVerificationTest("EqualsAndHashCode (structural equals + hash consistency)",
                                  passPath + "equals_hashcode_pass.mt");

        addOutputVerificationTest("Getter on final field",
                                  passPath + "getter_final_pass.mt");

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
