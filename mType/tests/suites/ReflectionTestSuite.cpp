#include "ReflectionTestSuite.hpp"

namespace tests::testSuite
{
    using namespace testFramework;

    void ReflectionTestSuite::setupTests()
    {
        // === CLASS INTROSPECTION TESTS ===
        // Tests for Class.forName and basic class metadata

        addOutputVerificationTest("Basic Reflection",
                        passPath + "basicReflection.mt");
        addOutputVerificationTest("Class forName",
                        passPath + "classForName.mt");
        addOutputVerificationTest("Class Metadata",
                        passPath + "classMetadata.mt");
        addOutputVerificationTest("Class Modifiers",
                        passPath + "classModifiers.mt");

        // === FIELD INTROSPECTION TESTS ===
        // Tests for field discovery and metadata

        addOutputVerificationTest("Field Discovery",
                        passPath + "fieldDiscovery.mt");
        addOutputVerificationTest("Field Modifiers",
                        passPath + "fieldModifiers.mt");
        addOutputVerificationTest("Field Types",
                        passPath + "fieldTypes.mt");
        addOutputVerificationTest("Static Field Access",
                        passPath + "staticFieldAccess.mt");

        // === METHOD INTROSPECTION TESTS ===
        // Tests for method discovery and metadata

        addOutputVerificationTest("Method Discovery",
                        passPath + "methodDiscovery.mt");
        addOutputVerificationTest("Method Modifiers",
                        passPath + "methodModifiers.mt");
        addOutputVerificationTest("Method Parameters",
                        passPath + "methodParameters.mt");
        addOutputVerificationTest("Method Return Types",
                        passPath + "methodReturnTypes.mt");

        // === CONSTRUCTOR INTROSPECTION TESTS ===
        // Tests for constructor discovery and metadata

        addOutputVerificationTest("Constructor Discovery",
                        passPath + "constructorDiscovery.mt");
        addOutputVerificationTest("Constructor Parameters",
                        passPath + "constructorParameters.mt");

        // === MODIFIER UTILITY TESTS ===
        // Tests for Modifier class utilities

        addOutputVerificationTest("Modifier Constants",
                        passPath + "modifierConstants.mt");
        addOutputVerificationTest("Modifier Utilities",
                        passPath + "modifierUtilities.mt");

        // === INHERITANCE REFLECTION TESTS ===
        // Tests for reflection on inherited members

        addOutputVerificationTest("Inheritance Reflection",
                        passPath + "inheritanceReflection.mt");
        addOutputVerificationTest("Superclass Reflection",
                        passPath + "superclassReflection.mt");

        // === INTERFACE REFLECTION TESTS ===
        // Tests for interface introspection

        addOutputVerificationTest("Interface Reflection",
                        passPath + "interfaceReflection.mt");

        // === GENERIC CLASS REFLECTION TESTS ===
        // Tests for reflection on generic classes

        addOutputVerificationTest("Generic Class Reflection",
                        passPath + "genericClassReflection.mt");

        // === INVOCATION TESTS ===
        // Tests for method invocation and constructor invocation via reflection

        addOutputVerificationTest("Reflection Invocation",
                        passPath + "reflectionInvocation.mt");

        // === ANNOTATION REFLECTION TESTS ===
        // Tests for annotation introspection

        addOutputVerificationTest("Annotation Reflection",
                        passPath + "annotationReflection.mt");

        // === ERROR TESTS ===
        // Tests for proper error handling in reflection

        addTestFromFile("Class Not Found Error",
                        errorPath + "classNotFound.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Field Not Found Error",
                        errorPath + "fieldNotFound.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Method Not Found Error",
                        errorPath + "methodNotFound.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Private Method Access Error",
                        errorPath + "privateMethodAccess.mt",
                        TestType::ERROR_EXPECTED);
    }
}
