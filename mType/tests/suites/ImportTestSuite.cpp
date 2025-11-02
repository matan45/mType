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
        // Array and Object Visibility Tests
        // ====================================
        addOutputVerificationTest("Import Public Arrays and Objects",
                        "mType/tests/testFiles/import/pass/test_public_array_object_import.mt");

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
        addTestFromFile("Import Private Arrays and Objects Error",
                        "mType/tests/testFiles/import/error/nested/test_private_array_object_access.mt",
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

        // ====================================
        // Advanced Selective Import Tests
        // ====================================
        addOutputVerificationTest("Multiple Selective Imports",
                        "mType/tests/testFiles/import/pass/importMultipleSelective.mt");
        addOutputVerificationTest("Selective Import With Alias",
                        "mType/tests/testFiles/import/pass/importSelectiveAlias.mt");
        addOutputVerificationTest("Mixed Selective Imports",
                        "mType/tests/testFiles/import/pass/importMixedSelective.mt");
        addOutputVerificationTest("Selective Import Ordering",
                        "mType/tests/testFiles/import/pass/importSelectiveOrdering.mt");
        addOutputVerificationTest("Selective Import Reexport",
                        "mType/tests/testFiles/import/pass/importSelectiveReexport.mt");

        // ====================================
        // Advanced Wildcard Import Tests
        // ====================================
        addOutputVerificationTest("Multiple Wildcard Imports",
                        "mType/tests/testFiles/import/pass/importMultipleWildcard.mt");
        addOutputVerificationTest("Wildcard With Shadowing",
                        "mType/tests/testFiles/import/pass/importWildcardShadowing.mt");
        addOutputVerificationTest("Nested Wildcard Imports",
                        "mType/tests/testFiles/import/pass/importNestedWildcard.mt");
        addOutputVerificationTest("Wildcard Name Collision Resolution",
                        "mType/tests/testFiles/import/pass/importWildcardCollision.mt");

        // ====================================
        // Namespace Import Tests
        // ====================================
        addOutputVerificationTest("Nested Namespace Import",
                        "mType/tests/testFiles/import/pass/importNestedNamespace.mt");
        addOutputVerificationTest("Deep Namespace Chain",
                        "mType/tests/testFiles/import/pass/importDeepNamespace.mt");
        addOutputVerificationTest("Cross Namespace Reference",
                        "mType/tests/testFiles/import/pass/importCrossNamespace.mt");
        addOutputVerificationTest("Namespace With Wildcard",
                        "mType/tests/testFiles/import/pass/importNamespaceWildcard.mt");

        // ====================================
        // Class Import Tests
        // ====================================
        addOutputVerificationTest("Import Class With Inheritance",
                        "mType/tests/testFiles/import/pass/importClassInheritance.mt");
        addOutputVerificationTest("Import Generic Class",
                        "mType/tests/testFiles/import/pass/importGenericClass.mt");
        addOutputVerificationTest("Import Abstract Class",
                        "mType/tests/testFiles/import/pass/importAbstractClass.mt");
        addOutputVerificationTest("Import Static Members",
                        "mType/tests/testFiles/import/pass/importStaticMembers.mt");
        addOutputVerificationTest("Import Nested Classes",
                        "mType/tests/testFiles/import/pass/importNestedClasses.mt");

        // ====================================
        // Interface Import Tests
        // ====================================
        addOutputVerificationTest("Import Interface",
                        "mType/tests/testFiles/import/pass/importInterface.mt");
        addOutputVerificationTest("Import Generic Interface",
                        "mType/tests/testFiles/import/pass/importGenericInterface.mt");
        addOutputVerificationTest("Import Interface Hierarchy",
                        "mType/tests/testFiles/import/pass/importInterfaceHierarchy.mt");
        addOutputVerificationTest("Import Lambda Compatible Interface",
                        "mType/tests/testFiles/import/pass/importLambdaInterface.mt");

        // ====================================
        // Function Import Tests
        // ====================================
        addOutputVerificationTest("Import Global Function",
                        "mType/tests/testFiles/import/pass/importGlobalFunction.mt");
        addOutputVerificationTest("Import Overloaded Functions",
                        "mType/tests/testFiles/import/pass/importOverloadedFunctions.mt");
        addOutputVerificationTest("Import Generic Function",
                        "mType/tests/testFiles/import/pass/importGenericFunction.mt");
        addOutputVerificationTest("Import Function With Default Args",
                        "mType/tests/testFiles/import/pass/importDefaultArgs.mt");

        // ====================================
        // Variable Import Tests
        // ====================================
        addOutputVerificationTest("Import Global Variables",
                        "mType/tests/testFiles/import/pass/importGlobalVariable.mt");
        addOutputVerificationTest("Import Constants",
                        "mType/tests/testFiles/import/pass/importConstants.mt");
        addOutputVerificationTest("Import Final Variables",
                        "mType/tests/testFiles/import/pass/importFinalVariables.mt");

        // ====================================
        // Complex Import Chain Tests
        // ====================================
        addOutputVerificationTest("Transitive Import Chain",
                        "mType/tests/testFiles/import/pass/importTransitiveChain.mt");
        addOutputVerificationTest("Diamond Import Pattern",
                        "mType/tests/testFiles/import/pass/importDiamondPattern.mt");
        addOutputVerificationTest("Import With Reexports",
                        "mType/tests/testFiles/import/pass/importReexports.mt");
        addOutputVerificationTest("Multi Level Import Cascade",
                        "mType/tests/testFiles/import/pass/importCascade.mt");

        // ====================================
        // Advanced Error Tests
        // ====================================
        addTestFromFile("Import Ambiguous Symbol Error",
                        "mType/tests/testFiles/import/error/importAmbiguousSymbol.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Import Circular Self Reference Error",
                        "mType/tests/testFiles/import/error/importCircularSelf.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Import Undefined Module Error",
                        "mType/tests/testFiles/import/error/importUndefinedModule.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Import Conflicting Types Error",
                        "mType/tests/testFiles/import/error/importConflictingTypes.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Import Protected Symbol Error",
                        "mType/tests/testFiles/import/error/importProtectedSymbol.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Import Invalid Path Error",
                        "mType/tests/testFiles/import/error/importInvalidPath.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Wildcard Import Ambiguity Error",
                        "mType/tests/testFiles/import/error/importWildcardAmbiguity.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Import Namespace Collision Error",
                        "mType/tests/testFiles/import/error/importNamespaceCollision.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Import Duplicate Selective Error",
                        "mType/tests/testFiles/import/error/importDuplicateSelective.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Import Generic Mismatch Error",
                        "mType/tests/testFiles/import/error/importGenericMismatch.mt",
                        TestType::ERROR_EXPECTED);
    }
}
