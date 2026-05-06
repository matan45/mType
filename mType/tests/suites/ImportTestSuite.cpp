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
        // ADDITIONAL TESTS - Files that exist but weren't registered
        // ====================================

        // === COLLECTION IMPORTS (4 tests) ===
        addOutputVerificationTest("Import All Collections",
                        "mType/tests/testFiles/import/pass/importAllCollections.mt");
        addOutputVerificationTest("Import HashMap Collection",
                        "mType/tests/testFiles/import/pass/importHashMapCollection.mt");
        addOutputVerificationTest("Import List Collection",
                        "mType/tests/testFiles/import/pass/importListCollection.mt");
        addOutputVerificationTest("Import Nested Collections",
                        "mType/tests/testFiles/import/pass/importNestedCollections.mt");
        addOutputVerificationTest("Import Random Math",
                        "mType/tests/testFiles/import/pass/importRandomMath.mt");

        // === GENERICS IMPORTS (6 tests) ===
        addOutputVerificationTest("Import Generic Class",
                        "mType/tests/testFiles/import/pass/importGenericClass.mt");
        addOutputVerificationTest("Import Generic Constraints",
                        "mType/tests/testFiles/import/pass/importGenericConstraints.mt");
        addOutputVerificationTest("Import Generic Interface",
                        "mType/tests/testFiles/import/pass/importGenericInterface.mt");
        addOutputVerificationTest("Import Generic Method",
                        "mType/tests/testFiles/import/pass/importGenericMethod.mt");
        addOutputVerificationTest("Import Nested Generics",
                        "mType/tests/testFiles/import/pass/importNestedGenerics.mt");
        addOutputVerificationTest("Import Specialized Generic",
                        "mType/tests/testFiles/import/pass/importSpecializedGeneric.mt");

        // === PRIMITIVE IMPORTS (3 tests) ===
        addOutputVerificationTest("Import Int Primitive",
                        "mType/tests/testFiles/import/pass/importIntPrimitive.mt");
        addOutputVerificationTest("Import String Primitive",
                        "mType/tests/testFiles/import/pass/importStringPrimitive.mt");
        addOutputVerificationTest("Import Multiple Primitives",
                        "mType/tests/testFiles/import/pass/importMultiplePrimitives.mt");

        // === PATH TESTS (6 tests) ===
        addOutputVerificationTest("Import Deep Nesting",
                        "mType/tests/testFiles/import/pass/importDeepNesting.mt");
        addOutputVerificationTest("Import Parent Directory",
                        "mType/tests/testFiles/import/pass/importParentDirectory.mt");
        addOutputVerificationTest("Import Relative Complex",
                        "mType/tests/testFiles/import/pass/importRelativeComplex.mt");
        addOutputVerificationTest("Import Same Directory",
                        "mType/tests/testFiles/import/pass/importSameDirectory.mt");
        addOutputVerificationTest("Import With Dot Slash",
                        "mType/tests/testFiles/import/pass/importWithDotSlash.mt");

        // MYT-278: cross-file static method returning typed array, returned directly
        addOutputVerificationTest("Import Static Array Return",
                        "mType/tests/testFiles/import/pass/importStaticArrayReturn.mt");
        // MYT-278 (negative): returning string[] from an int[]-typed function must error
        addTestFromFile("Import Static Array Wrong Type Return Error",
                        "mType/tests/testFiles/import/error/importStaticArrayWrongTypeReturn_error.mt",
                        TestType::ERROR_EXPECTED);

        // === DEPENDENCY TESTS (4 tests) ===
        addOutputVerificationTest("Import Diamond Dependency",
                        "mType/tests/testFiles/import/pass/importDiamondDependency.mt");
        addOutputVerificationTest("Import Reexport",
                        "mType/tests/testFiles/import/pass/importReexport.mt");
        addOutputVerificationTest("Import Redundant",
                        "mType/tests/testFiles/import/pass/importRedundant.mt");

        // === OVERLOADED STATIC METHOD IMPORTS (4 tests) ===
        addOutputVerificationTest("Import Overloaded Static Methods",
                        "mType/tests/testFiles/import/pass/importOverloadedStaticMethods.mt");
        addOutputVerificationTest("Import Overloaded Static Wildcard",
                        "mType/tests/testFiles/import/pass/importOverloadedStaticWildcard.mt");
        addOutputVerificationTest("Import Multi Class Overloaded Static",
                        "mType/tests/testFiles/import/pass/importMultiClassOverloadedStatic.mt");
        addOutputVerificationTest("Import Overloaded Static Object Param",
                        "mType/tests/testFiles/import/pass/importOverloadedStaticObjectParam.mt");

        // === SPECIAL TESTS (5 tests) ===
        addOutputVerificationTest("Import Empty File",
                        "mType/tests/testFiles/import/pass/importEmptyFile.mt");
        addOutputVerificationTest("Import Object Base",
                        "mType/tests/testFiles/import/pass/importObjectBase.mt");
        addOutputVerificationTest("Import Private Only",
                        "mType/tests/testFiles/import/pass/importPrivateOnly.mt");
        addOutputVerificationTest("Import Selective And Wildcard",
                        "mType/tests/testFiles/import/pass/importSelectiveAndWildcard.mt");

        // === ERROR TESTS THAT EXIST ===
        addTestFromFile("Import Circular Self Reference Error",
                        "mType/tests/testFiles/import/error/importCircularSelf.mt",
                        TestType::ERROR_EXPECTED);

        // ====================================
        // COMMENTED OUT - Test files were not created
        // ====================================
        /*
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
        addOutputVerificationTest("Multiple Wildcard Imports",
                        "mType/tests/testFiles/import/pass/importMultipleWildcard.mt");
        addOutputVerificationTest("Wildcard With Shadowing",
                        "mType/tests/testFiles/import/pass/importWildcardShadowing.mt");
        addOutputVerificationTest("Nested Wildcard Imports",
                        "mType/tests/testFiles/import/pass/importNestedWildcard.mt");
        addOutputVerificationTest("Wildcard Name Collision Resolution",
                        "mType/tests/testFiles/import/pass/importWildcardCollision.mt");
        addOutputVerificationTest("Nested Namespace Import",
                        "mType/tests/testFiles/import/pass/importNestedNamespace.mt");
        addOutputVerificationTest("Deep Namespace Chain",
                        "mType/tests/testFiles/import/pass/importDeepNamespace.mt");
        addOutputVerificationTest("Cross Namespace Reference",
                        "mType/tests/testFiles/import/pass/importCrossNamespace.mt");
        addOutputVerificationTest("Namespace With Wildcard",
                        "mType/tests/testFiles/import/pass/importNamespaceWildcard.mt");
        addOutputVerificationTest("Import Class With Inheritance",
                        "mType/tests/testFiles/import/pass/importClassInheritance.mt");
        addOutputVerificationTest("Import Abstract Class",
                        "mType/tests/testFiles/import/pass/importAbstractClass.mt");
        addOutputVerificationTest("Import Static Members",
                        "mType/tests/testFiles/import/pass/importStaticMembers.mt");
        addOutputVerificationTest("Import Nested Classes",
                        "mType/tests/testFiles/import/pass/importNestedClasses.mt");
        addOutputVerificationTest("Import Interface",
                        "mType/tests/testFiles/import/pass/importInterface.mt");
        addOutputVerificationTest("Import Interface Hierarchy",
                        "mType/tests/testFiles/import/pass/importInterfaceHierarchy.mt");
        addOutputVerificationTest("Import Lambda Compatible Interface",
                        "mType/tests/testFiles/import/pass/importLambdaInterface.mt");
        addOutputVerificationTest("Import Global Function",
                        "mType/tests/testFiles/import/pass/importGlobalFunction.mt");
        addOutputVerificationTest("Import Overloaded Functions",
                        "mType/tests/testFiles/import/pass/importOverloadedFunctions.mt");
        addOutputVerificationTest("Import Generic Function",
                        "mType/tests/testFiles/import/pass/importGenericFunction.mt");
        addOutputVerificationTest("Import Function With Default Args",
                        "mType/tests/testFiles/import/pass/importDefaultArgs.mt");
        addOutputVerificationTest("Import Global Variables",
                        "mType/tests/testFiles/import/pass/importGlobalVariable.mt");
        addOutputVerificationTest("Import Constants",
                        "mType/tests/testFiles/import/pass/importConstants.mt");
        addOutputVerificationTest("Import Final Variables",
                        "mType/tests/testFiles/import/pass/importFinalVariables.mt");
        addOutputVerificationTest("Transitive Import Chain",
                        "mType/tests/testFiles/import/pass/importTransitiveChain.mt");
        addOutputVerificationTest("Diamond Import Pattern",
                        "mType/tests/testFiles/import/pass/importDiamondPattern.mt");
        addOutputVerificationTest("Import With Reexports",
                        "mType/tests/testFiles/import/pass/importReexports.mt");
        addOutputVerificationTest("Multi Level Import Cascade",
                        "mType/tests/testFiles/import/pass/importCascade.mt");
        addTestFromFile("Import Ambiguous Symbol Error",
                        "mType/tests/testFiles/import/error/importAmbiguousSymbol.mt",
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
        */

        // ====================================
        // Library Import (import lib) Tests
        // ====================================
        addOutputVerificationTest("Import Lib - Wildcard Syntax",
                        "mType/tests/testFiles/import/pass/library/importLibWildcard.mt");
        addOutputVerificationTest("Import Lib - Selective Syntax",
                        "mType/tests/testFiles/import/pass/library/importLibSelective.mt");
        addOutputVerificationTest("Import Lib - Mixed With Source Imports",
                        "mType/tests/testFiles/import/pass/library/importLibMixedWithSource.mt");

        addOutputVerificationTest("Import Lib - Alias Syntax",
                        "mType/tests/testFiles/import/pass/library/importLibAlias.mt");
        addOutputVerificationTest("Import Source - Alias Syntax",
                        "mType/tests/testFiles/import/pass/library/importSourceAlias.mt");
        addOutputVerificationTest("Import Source - Multi Alias",
                        "mType/tests/testFiles/import/pass/library/importSourceMultiAlias.mt");
        addOutputVerificationTest("Import Interface - Alias Syntax",
                        "mType/tests/testFiles/import/pass/library/importInterfaceAlias.mt");

        // Library Import Error Tests
        addTestFromFile("Import Lib - Empty Name Error",
                        "mType/tests/testFiles/import/error/library/importLibEmptyName.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Import Lib - Empty Selective List Error",
                        "mType/tests/testFiles/import/error/library/importLibSelectiveEmpty.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Import Lib - Missing Semicolon Error",
                        "mType/tests/testFiles/import/error/library/importLibMissingSemicolon.mt",
                        TestType::ERROR_EXPECTED);
    }
}
