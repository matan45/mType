#include "ImportTestSuite.hpp"
#include "../testFramework/TestTypeEnum.hpp"

namespace tests::testSuite
{
    using namespace testFramework;

    void ImportTestSuite::setupTests()
    {
        // ====================================
        // Selective Import Tests
        // ====================================
        addOutputVerificationTest("Selective Import - Basic",
                        "mType/tests/testFiles/import/pass/test_selective_import.mt");

        // ====================================
        // Wildcard Import Tests
        // ====================================
        addOutputVerificationTest("Wildcard Import - Basic",
                        "mType/tests/testFiles/import/pass/test_wildcard_import.mt");

        // ====================================
        // Nested Multi-Level Import Tests
        // ====================================
        addOutputVerificationTest("Nested Import - Selective Chain",
                        "mType/tests/testFiles/import/pass/nested/test_nested_imports.mt");
        addOutputVerificationTest("Nested Import - Wildcard Chain",
                        "mType/tests/testFiles/import/pass/nested/test_wildcard_nested_imports.mt");

        // ====================================
        // Visibility Error Tests
        // ====================================
        addTestFromFile("Import Private Symbol Error",
                        "mType/tests/testFiles/import/error/test_import_private_symbol.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Import Private Class Error",
                        "mType/tests/testFiles/import/error/test_import_private_class.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Import Non-Existent Symbol Error",
                        "mType/tests/testFiles/import/error/test_import_nonexistent_symbol.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Wildcard Import No Private Access Error",
                        "mType/tests/testFiles/import/error/test_wildcard_no_access_private.mt",
                        TestType::ERROR_EXPECTED);

        // ====================================
        // Nested Visibility Error Tests
        // ====================================
        addTestFromFile("Nested Private Access Error",
                        "mType/tests/testFiles/import/error/nested/test_nested_private_access.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Transitive Private Access Error",
                        "mType/tests/testFiles/import/error/nested/test_transitive_private.mt",
                        TestType::ERROR_EXPECTED);

        // ====================================
        // Circular Dependency Tests
        // ====================================
        addTestFromFile("Circular Import - Two Way",
                        "mType/tests/testFiles/import/error/circular_module_a.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Circular Import - Three Way",
                        "mType/tests/testFiles/import/error/circular_three_way_a.mt",
                        TestType::ERROR_EXPECTED);
    }
}
