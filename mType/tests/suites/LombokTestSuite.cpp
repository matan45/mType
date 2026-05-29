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

        addOutputVerificationTest("Builder with a nested @Builder object field",
                                  passPath + "builder_nested_pass.mt");

        addOutputVerificationTest("EqualsAndHashCode (structural equals + hash consistency)",
                                  passPath + "equals_hashcode_pass.mt");

        addOutputVerificationTest("Getter on final field",
                                  passPath + "getter_final_pass.mt");

        addOutputVerificationTest("AllArgsConstructor inheritance (super forwarding)",
                                  passPath + "allargs_inheritance_pass.mt");

        addOutputVerificationTest("Object-typed getters + nested toString",
                                  passPath + "object_fields_pass.mt");

        addOutputVerificationTest("ToString renders a null object field as \"null\"",
                                  passPath + "tostring_null_object_pass.mt");

        addOutputVerificationTest("Data with a string field — structural equals/hashCode",
                                  passPath + "data_string_equals_pass.mt");

        addOutputVerificationTest("EqualsAndHashCode over object fields (recursive equals)",
                                  passPath + "equals_hashcode_object_pass.mt");

        addOutputVerificationTest("Field-level @Getter — only annotated fields get accessors",
                                  passPath + "field_level_getter_pass.mt");

        addOutputVerificationTest("Data with only non-final fields (no-arg ctor)",
                                  passPath + "noargs_data_combo_pass.mt");

        addOutputVerificationTest("Skip-if-exists: user getter not overwritten",
                                  passPath + "skip_existing_getter_pass.mt");

        addOutputVerificationTest("ToString over all primitive field types",
                                  passPath + "tostring_primitives_pass.mt");

        addOutputVerificationTest("Getter over an array-typed field",
                                  passPath + "getter_array_pass.mt");

        addOutputVerificationTest("NoArgsConstructor chains to parent (super())",
                                  passPath + "noargs_inheritance_pass.mt");

        addOutputVerificationTest("Skip-if-exists: user toString not overwritten",
                                  passPath + "skip_existing_tostring_pass.mt");

        addOutputVerificationTest("Skip-if-exists: user setter not overwritten",
                                  passPath + "skip_existing_setter_pass.mt");

        // --- JIT / OSR over synthesized methods ---

        addOutputVerificationTest("Synthesized getter/setter in a hot loop (JIT/OSR)",
                                  passPath + "getter_setter_hotloop_pass.mt");

        // ===== ERROR TESTS =====

        addTestFromFile("@Getter on function rejected (@Target)",
                        errorPath + "getter_on_function_error.mt",
                        TestType::ERROR_EXPECTED,
                        "cannot be applied to");

        addTestFromFile("@Data on function rejected (@Target)",
                        errorPath + "data_on_function_error.mt",
                        TestType::ERROR_EXPECTED,
                        "cannot be applied to");

        // Skip-error canaries assert the SPECIFIC unresolved accessor (the
        // method-not-found message embeds the method name) rather than passing
        // on any error — so an unrelated parse/typecheck failure can't mask a
        // regression where synthesis wrongly fires for a skipped class shape.
        addTestFromFile("Generic class skipped — getter not generated",
                        errorPath + "generic_class_skip_error.mt",
                        TestType::ERROR_EXPECTED,
                        "getValue");

        addTestFromFile("Value class skipped — getter not generated",
                        errorPath + "value_class_skip_error.mt",
                        TestType::ERROR_EXPECTED,
                        "getCents");

        addTestFromFile("Abstract class skipped — getter not generated",
                        errorPath + "abstract_class_skip_error.mt",
                        TestType::ERROR_EXPECTED,
                        "getSides");

        addTestFromFile("Setter skips final field — no setter generated",
                        errorPath + "setter_final_skip_error.mt",
                        TestType::ERROR_EXPECTED,
                        "setX");

        addTestFromFile("@Builder on function rejected (@Target)",
                        errorPath + "builder_on_function_error.mt",
                        TestType::ERROR_EXPECTED,
                        "cannot be applied to");

        addTestFromFile("@Setter on function rejected (@Target)",
                        errorPath + "setter_on_function_error.mt",
                        TestType::ERROR_EXPECTED,
                        "cannot be applied to");
    }
}
