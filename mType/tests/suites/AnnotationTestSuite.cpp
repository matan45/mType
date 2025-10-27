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

        // ===== MULTIPLE ANNOTATIONS - PASS TESTS =====
        // Tests for using multiple annotations together

        addOutputVerificationTest("Multiple Annotations Combined",
                        passPath + "multiple_annotations_pass.mt");

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
    }
}
