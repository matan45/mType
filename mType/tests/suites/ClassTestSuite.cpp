#include "ClassTestSuite.hpp"

namespace tests::testSuite
{
    using namespace testFramework;

    void ClassTestSuite::setupTests()
    {
        // Basic class functionality tests
        addOutputVerificationTest("Basic Class Definition",
                        passPath + "basicClassDefinition.mt");
        addOutputVerificationTest("Class with Data Members",
                        passPath + "classWithDataMembers.mt");
        addOutputVerificationTest("Class with Methods",
                        passPath + "classWithMethods.mt");
        addOutputVerificationTest("Multiple Constructors",
                        passPath + "multipleConstructors.mt");
        addOutputVerificationTest("Default Constructor",
                        passPath + "defaultConstructor.mt");
        addOutputVerificationTest("Constructor with Parameters",
                        passPath + "constructorWithParameters.mt");

        // Static members and methods tests
        addOutputVerificationTest("Static Data Members",
                        passPath + "staticDataMembers.mt");
        addOutputVerificationTest("Static Methods",
                        passPath + "staticMethods.mt");
        addOutputVerificationTest("Static Final Members",
                        passPath + "staticFinalMembers.mt");
        addOutputVerificationTest("Mixed Static and Instance",
                        passPath + "mixedStaticAndInstance.mt");

        // Member access and object lifecycle tests
        addOutputVerificationTest("Member Access",
                        passPath + "memberAccess.mt");
        addOutputVerificationTest("Method Calls",
                        passPath + "methodCalls.mt");
        addOutputVerificationTest("Chained Member Access",
                        passPath + "chainedMemberAccess.mt");
        addOutputVerificationTest("Member Assignment",
                        passPath + "memberAssignment.mt");
        addOutputVerificationTest("Object Creation",
                        passPath + "objectCreation.mt");
        addOutputVerificationTest("Null Handling",
                        passPath + "nullHandling.mt");
        addOutputVerificationTest("Object Assignment",
                        passPath + "objectAssignment.mt");
        addOutputVerificationTest("Multiple Objects",
                        passPath + "multipleObjects.mt");

        // Complex scenarios tests
        addOutputVerificationTest("Class with All Features",
                        passPath + "classWithAllFeatures.mt");
        addOutputVerificationTest("Class in Functions",
                        passPath + "classInFunctions.mt");
        addOutputVerificationTest("Class as Parameter",
                        passPath + "classAsParameter.mt");
        addOutputVerificationTest("Class as Return Type",
                        passPath + "classAsReturnType.mt");
        addOutputVerificationTest("Returning This and Method Chaining",
                        passPath + "returningThisAndChaining.mt");
        addOutputVerificationTest("Complex Example",
                        passPath + "complexExample.mt");

        // Exception handling tests
        addOutputVerificationTest("Try-Catch-Finally with Return",
                        passPath + "tryCatchFinallyWithReturn.mt");
        addOutputVerificationTest("Exception in Finally Block",
                        passPath + "exceptionInFinally.mt");

        // Object reference edge case tests
        addOutputVerificationTest("Circular Object References",
                        passPath + "circularObjectReferences.mt");

        // === INHERITANCE AND POLYMORPHISM TESTS ===
        // These tests verify class inheritance, method overriding, and polymorphism

        addOutputVerificationTest("Basic Inheritance",
                        passPath + "basicInheritance.mt");
        addOutputVerificationTest("Method Override",
                        passPath + "methodOverride.mt");
        addOutputVerificationTest("Constructor Chaining",
                        passPath + "constructorChaining.mt");
        addOutputVerificationTest("Polymorphism",
                        passPath + "polymorphism.mt");
        addOutputVerificationTest("Deep Inheritance",
                        passPath + "deepInheritance.mt");
        addOutputVerificationTest("Generic Inheritance",
                        passPath + "genericInheritance.mt");

        // === FINAL CLASS TESTS ===
        // These tests verify final class functionality

        addOutputVerificationTest("Final Class Definition",
                        passPath + "finalClassDefinition.mt");
        addOutputVerificationTest("Final Class With Members",
                        passPath + "finalClassWithMembers.mt");
        addOutputVerificationTest("Final Generic Class",
                        passPath + "finalGenericClass.mt");

        // === LEXICAL SCOPING TESTS ===
        // These tests verify that mType implements proper lexical scoping

        // Method scoping tests
        addOutputVerificationTest("Method Scoping - Isolation",
                        passPath + "methodScopingPass.mt");

        // Static method and field scoping tests
        addOutputVerificationTest("Static Method and Field Scoping",
                        passPath + "staticScopingPass.mt");

        // Loop scoping tests
        addOutputVerificationTest("Loop Variable Scoping",
                        passPath + "loopScopingPass.mt");

        // Conditional scoping tests
        addOutputVerificationTest("Conditional Block Scoping",
                        passPath + "conditionalScopingPass.mt");

        // Function scoping tests
        addOutputVerificationTest("Function Lexical Scoping",
                        passPath + "functionScopingPass.mt");

        // Error tests (expected to fail)
        addTestFromFile("Null Pointer Access Error",
                        errorPath + "nullPointerAccess.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Undefined Class Error",
                        errorPath + "undefinedClass.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Invalid Constructor Error",
                        errorPath + "invalidConstructor.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Access Non-Existent Member Error",
                        errorPath + "accessNonExistentMember.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Static Class Declaration Error",
                        errorPath + "staticClassDeclaration.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Static Constructor Error",
                        errorPath + "staticConstructor.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Final Constructor Error",
                        errorPath + "finalConstructor.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Final Method Error",
                        errorPath + "finalMethod.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Static Parameters Error",
                        errorPath + "staticParameters.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Final Parameters Error",
                        errorPath + "finalParameters.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Static Return Types Error",
                        errorPath + "staticReturnTypes.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Final Return Types Error",
                        errorPath + "finalReturnTypes.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Constructor Return Types Error",
                        errorPath + "constructorReturnTypes.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Constructor Return Types String Error",
                        errorPath + "constructorReturnTypesString.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Constructor Return Types Void Error",
                        errorPath + "constructorReturnTypesVoid.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Final Variable Reassignment Error",
                        errorPath + "finalVariableReassignment.mt",
                        TestType::ERROR_EXPECTED);

        // === LEXICAL SCOPING ERROR TESTS ===
        // These tests verify that improper scoping is correctly rejected

        addTestFromFile("Method Cross-Scoping Error",
                        errorPath + "methodScopingFail.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Static Method Instance Access Error",
                        errorPath + "staticScopingFail.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Loop Variable Out-of-Scope Error",
                        errorPath + "loopScopingFail.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Conditional Block Out-of-Scope Error",
                        errorPath + "conditionalScopingFail.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Function Cross-Scoping Error",
                        errorPath + "functionScopingFail.mt",
                        TestType::ERROR_EXPECTED);

        // === INHERITANCE ERROR TESTS ===
        // These tests verify that inheritance errors are correctly detected

        addTestFromFile("Circular Inheritance Error",
                        errorPath + "circularInheritance.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Parent Not Found Error",
                        errorPath + "parentNotFound.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Invalid Override Error",
                        errorPath + "invalidOverride.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Missing Super Call Error",
                        errorPath + "missingSuper.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Invalid Super Placement Error",
                        errorPath + "invalidSuper.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Multiple Inheritance Error",
                        errorPath + "multipleInheritance.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("super syntax",
                        errorPath + "superSyntax.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Class Extends Interface Error",
                        errorPath + "classExtendsInterface.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Extends Final Class Error",
                        errorPath + "extendsFinalClass.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Extends Final Generic Class Error",
                        errorPath + "extendsFinalGenericClass.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("duplicate catch test",
                        errorPath + "duplicate_catch_test.mt",
                        TestType::ERROR_EXPECTED);

        // === NESTED TYPE VALIDATION TESTS ===
        // These tests verify that nested classes and interfaces are properly rejected

        addTestFromFile("Nested Class in Class Error",
                        "mType/tests/testFiles/error/error/nestedClassInClass.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Nested Interface in Class Error",
                        "mType/tests/testFiles/error/error/nestedInterfaceInClass.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Nested Class in Interface Error",
                        "mType/tests/testFiles/error/error/nestedClassInInterface.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Nested Interface in Interface Error",
                        "mType/tests/testFiles/error/error/nestedInterfaceInInterface.mt",
                        TestType::ERROR_EXPECTED);
    }
}
