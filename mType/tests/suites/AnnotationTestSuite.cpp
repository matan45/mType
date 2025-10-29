#include "AnnotationTestSuite.hpp"

namespace tests::testSuite
{
    using namespace testFramework;

    void AnnotationTestSuite::setupTests()
    {
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
    }
}
