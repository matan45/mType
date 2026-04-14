#include "AnnotationTestSuite.hpp"

namespace tests::testSuite
{
    using namespace testFramework;

    void AnnotationTestSuite::setupTests()
    {
        // ===== USER-DEFINED ANNOTATIONS (MYT-108) - PASS TESTS =====

        addOutputVerificationTest("Annotation Declaration - Empty Body",
                                  passPath + "annotation_declare_empty_pass.mt");

        addOutputVerificationTest("Annotation Declaration - Int Param + Reflection",
                                  passPath + "annotation_declare_int_pass.mt");

        addOutputVerificationTest("Annotation Declaration - String Positional Shorthand",
                                  passPath + "annotation_declare_string_pass.mt");

        addOutputVerificationTest("Annotation Declaration - Partial Defaults",
                                  passPath + "annotation_defaults_partial_pass.mt");

        addOutputVerificationTest("Annotation On Field (instance + static)",
                                  passPath + "annotation_on_field_pass.mt");

        addOutputVerificationTest("Annotation On Constructor",
                                  passPath + "annotation_on_constructor_pass.mt");

        addOutputVerificationTest("Annotation On Top-Level Function",
                                  passPath + "annotation_on_top_function_pass.mt");

        // ===== USER-DEFINED ANNOTATIONS (MYT-108) - ERROR TESTS =====

        addTestFromFile("Annotation Usage - Unknown Annotation",
                        errorPath + "unknown_annotation_error.mt",
                        TestType::ERROR_EXPECTED,
                        "Unknown annotation");

        addTestFromFile("Annotation Usage - Wrong Param Type",
                        errorPath + "wrong_param_type_error.mt",
                        TestType::ERROR_EXPECTED,
                        "expects int");

        addTestFromFile("Annotation Usage - Missing Required Param",
                        errorPath + "missing_required_param_error.mt",
                        TestType::ERROR_EXPECTED,
                        "missing required parameter");

        addTestFromFile("Annotation Usage - Unknown Param Name",
                        errorPath + "unknown_param_name_error.mt",
                        TestType::ERROR_EXPECTED,
                        "Unknown parameter");

        addTestFromFile("Annotation Usage - Unknown Annotation On Function",
                        errorPath + "unknown_annotation_on_function_error.mt",
                        TestType::ERROR_EXPECTED,
                        "Unknown annotation");

        // ===== @Override ANNOTATION - PASS TESTS =====
        // Tests for valid @Override usage with parent classes and interfaces

        addOutputVerificationTest("Override Parent Class Method",
                                  passPath + "override_parent_class_pass.mt");

        addOutputVerificationTest("Override Interface Method",
                                  passPath + "override_interface_pass.mt");

        addOutputVerificationTest("Override Grandparent Method",
                                  passPath + "override_grandparent_pass.mt");

        // ===== @Script ANNOTATION - PASS TESTS =====
        // Tests for @Script annotation marking classes for C++ binding

        addOutputVerificationTest("Script Annotation on Classes",
                                  passPath + "script_annotation_pass.mt");

        addOutputVerificationTest("Script Annotation Valid Requirements",
                                  passPath + "script_annotation_valid.mt");

        // ===== @Script C++ INTEROP - PASS TESTS =====
        // Tests createObject + callMethod path (reproduces MYT-25 regression)

        addInteropTest("Script C++ Interop - createObject + callMethod",
                       passPath + "script_callmethod_test.mt");

        addInteropTest("Script C++ Interop - Native String Comparison",
                       passPath + "script_native_string_compare_test.mt");

        addInteropTest("Script C++ Interop - Async callMethod",
                       passPath + "script_async_callmethod_test.mt");

        // ===== @EntryPoint ANNOTATION - PASS TESTS =====
        // Tests for @EntryPoint annotation marking classes as executable entry points

        addOutputVerificationTest("EntryPoint Annotation on Class",
                                  passPath + "entrypoint_annotation_pass.mt");

        addOutputVerificationTest("EntryPoint Print Args",
                                  passPath + "entrypoint_print_args_pass.mt");

        // ===== MULTIPLE ANNOTATIONS - PASS TESTS =====
        // Tests for using multiple annotations together

        addOutputVerificationTest("Multiple Annotations Combined",
                                  passPath + "multiple_annotations_pass.mt");

        // ===== @Throw ANNOTATION - PASS TESTS =====
        // Tests for valid @Throw usage with exception declarations

        addOutputVerificationTest("Throw Single Exception",
                                  passPath + "single_exception_pass.mt");

        addOutputVerificationTest("Throw Multiple Exceptions",
                                  passPath + "multiple_exceptions_pass.mt");

        addOutputVerificationTest("Throw on Method",
                                  passPath + "method_throw_pass.mt");

        addOutputVerificationTest("Throw Combined with Override",
                                  passPath + "combined_annotations_pass.mt");

        // ===== @Override ANNOTATION - ERROR TESTS =====
        // Tests for invalid @Override usage that should fail compilation

        addTestFromFile("Override Without Parent Method",
                        errorPath + "override_no_parent_method_error.mt",
                        TestType::ERROR_EXPECTED);

        addTestFromFile("Override With Wrong Signature",
                        errorPath + "override_wrong_signature_error.mt",
                        TestType::ERROR_EXPECTED);

        addTestFromFile("Override With Wrong Parameter Count",
                        errorPath + "override_wrong_param_count_error.mt",
                        TestType::ERROR_EXPECTED);

        addTestFromFile("Override Without Parent Class",
                        errorPath + "override_no_parent_error.mt",
                        TestType::ERROR_EXPECTED);

        addTestFromFile("Override With Wrong Return Type",
                        errorPath + "override_wrong_return_type_error.mt",
                        TestType::ERROR_EXPECTED);

        // ===== @Script ANNOTATION - ERROR TESTS =====
        // Tests for invalid @Script usage that should fail compilation

        addTestFromFile("Script on Abstract Class",
                        errorPath + "script_annotation_abstract_error.mt",
                        TestType::ERROR_EXPECTED);

        addTestFromFile("Script Without Default Constructor",
                        errorPath + "script_annotation_no_default_constructor_error.mt",
                        TestType::ERROR_EXPECTED);

        addTestFromFile("Script Without Update Method",
                        errorPath + "script_annotation_no_update_method_error.mt",
                        TestType::ERROR_EXPECTED);

        addTestFromFile("Script With Wrong Update Signature",
                        errorPath + "script_annotation_wrong_update_signature_error.mt",
                        TestType::ERROR_EXPECTED);

        addTestFromFile("Script Without Start Method",
                        errorPath + "script_annotation_no_start_method_error.mt",
                        TestType::ERROR_EXPECTED);

        addTestFromFile("Script Without Clean Method",
                        errorPath + "script_annotation_no_clean_method_error.mt",
                        TestType::ERROR_EXPECTED);

        // ===== @EntryPoint ANNOTATION - ERROR TESTS =====
        // Tests for invalid @EntryPoint usage that should fail compilation

        addTestFromFile("EntryPoint on Abstract Class",
                        errorPath + "entrypoint_abstract_error.mt",
                        TestType::ERROR_EXPECTED);

        addTestFromFile("EntryPoint Without Main Method",
                        errorPath + "entrypoint_no_main_error.mt",
                        TestType::ERROR_EXPECTED);

        // ===== @Throw ANNOTATION - ERROR TESTS =====
        // Tests for invalid @Throw usage that should fail compilation

        addTestFromFile("Throw Invalid Exception Class",
                        errorPath + "invalid_exception_class_error.mt",
                        TestType::ERROR_EXPECTED);

        addTestFromFile("Throw Non-Exception Class",
                        errorPath + "non_exception_class_error.mt",
                        TestType::ERROR_EXPECTED);

        addTestFromFile("Throw Empty Parameter List",
                        errorPath + "empty_throw_error.mt",
                        TestType::ERROR_EXPECTED);

        // ===== MYT-109 (3a): META-ANNOTATIONS - PASS TESTS =====

        addOutputVerificationTest("Meta-annotation Declaration Parses",
                                  passPath + "meta_annotation_decl_pass.mt");

        addOutputVerificationTest("Meta-annotation @Retention(SOURCE) Stripped",
                                  passPath + "meta_retention_source_stripped_pass.mt");

        addOutputVerificationTest("Constructor Annotation Reflection",
                                  passPath + "annotation_on_ctor_reflection_pass.mt");

        // ===== MYT-109 (3a): META-ANNOTATIONS - ERROR TESTS =====

        addTestFromFile("Meta-annotation @Target Violation",
                        errorPath + "meta_target_violation_error.mt",
                        TestType::ERROR_EXPECTED,
                        "cannot be applied to");
    }
}
