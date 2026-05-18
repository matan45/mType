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
        addOutputVerificationTest("Method Parameter Count Excludes This (MYT-214)",
                        passPath + "getParameterCountExcludesThis.mt");
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

        // === PARAMETERIZED TYPE REFLECTION TESTS (MYT-40) ===
        // Tests that Class.forName("Box<Int>") and getTypeArguments() surface
        // the reified type arguments of closed parameterized generic classes.

        addOutputVerificationTest("Get Type Arguments Basic",
                        passPath + "getTypeArgumentsBasic.mt");
        addOutputVerificationTest("Get Type Arguments Are Classes",
                        passPath + "getTypeArgumentsAreClasses.mt");
        addOutputVerificationTest("Get Type Arguments Multiple Params",
                        passPath + "getTypeArgumentsMultipleParams.mt");
        addOutputVerificationTest("Get Type Arguments Aligned With Parameters",
                        passPath + "getTypeArgumentsAlignedWithParameters.mt");
        addOutputVerificationTest("Get Type Arguments Empty For Open",
                        passPath + "getTypeArgumentsEmptyForOpen.mt");
        addOutputVerificationTest("Get Type Arguments Empty For Non Generic",
                        passPath + "getTypeArgumentsEmptyForNonGeneric.mt");
        addOutputVerificationTest("Class Get Name Parameterized",
                        passPath + "classGetNameParameterized.mt");
        addOutputVerificationTest("Class Get Name Nested",
                        passPath + "classGetNameNested.mt");
        addOutputVerificationTest("Open Vs Closed Not Equal",
                        passPath + "openVsClosedNotEqual.mt");
        addOutputVerificationTest("For Name Identity Convergence",
                        passPath + "forNameIdentityConvergence.mt");
        addOutputVerificationTest("Get Class Name Still Raw",
                        passPath + "getClassNameStillRaw.mt");

        // Box<T>-centric coverage — drives the parser, interning, and
        // nested-generic code paths through a single shared generic class.
        addOutputVerificationTest("Box Template Reflection",
                        passPath + "boxTemplateReflection.mt");
        addOutputVerificationTest("Box Multiple Primitives",
                        passPath + "boxMultiplePrimitives.mt");
        addOutputVerificationTest("Box Of Box",
                        passPath + "boxOfBox.mt");
        addOutputVerificationTest("Box User Class",
                        passPath + "boxUserClass.mt");

        // === INVOCATION TESTS ===
        // Tests for method invocation and constructor invocation via reflection

        addOutputVerificationTest("Reflection Invocation",
                        passPath + "reflectionInvocation.mt");
        addOutputVerificationTest("New Instance With Multiple Args",
                        passPath + "newInstanceWithMultipleArgs.mt");

        // === MYT-111 REGRESSION TESTS ===
        // Exception propagation across reflective invocation boundaries.
        addOutputVerificationTest("Reflect Invoke Method Throws Caught By Caller",
                        passPath + "reflectInvokeMethodThrowsCaughtByCaller.mt");
        addOutputVerificationTest("Reflect New Instance Throws Caught By Caller",
                        passPath + "reflectNewInstanceThrowsCaughtByCaller.mt");

        // === FIELD GET/SET TESTS ===
        // Round-trip and static-write paths (Field.get/set, setStaticFieldValue)
        addOutputVerificationTest("Field Get Set Instance",
                        passPath + "fieldGetSetInstance.mt");
        addOutputVerificationTest("Static Field Set",
                        passPath + "staticFieldSet.mt");
        addOutputVerificationTest("Set Field Wrong Type Silent (doc pin)",
                        passPath + "setFieldWrongTypeSilent.mt");

        // === ACCESSIBLE OBJECT TESTS ===
        addOutputVerificationTest("Set Accessible Round Trip",
                        passPath + "setAccessibleRoundTrip.mt");

        // === INTERFACE METHOD REFLECTION ===
        addOutputVerificationTest("Interface Method Reflection",
                        passPath + "interfaceMethodReflection.mt");

        // === ANNOTATION REFLECTION - DEPTH ===
        addOutputVerificationTest("Class Get Annotation Missing Returns Null",
                        passPath + "classGetAnnotationMissingReturnsNull.mt");
        addOutputVerificationTest("Method Get Annotations Array",
                        passPath + "methodGetAnnotationsArray.mt");
        addOutputVerificationTest("Constructor Get Annotations Array",
                        passPath + "constructorGetAnnotationsArray.mt");
        addOutputVerificationTest("Annotation Typed Params All Variants",
                        passPath + "annotationTypedParamsAllVariants.mt");
        addOutputVerificationTest("Meta Annotation Read Params",
                        passPath + "metaAnnotationReadParams.mt");

        // === OBJECT-BASED REFLECTION API TESTS ===
        // Tests for simplified reflection using Object as universal type

        addOutputVerificationTest("Object Reflection API",
                        passPath + "objectReflectionApi.mt");

        // === ANNOTATION REFLECTION TESTS ===
        // Tests for annotation introspection

        addOutputVerificationTest("Annotation Reflection",
                        passPath + "annotationReflection.mt");

        // === ERROR TESTS ===
        // Tests for proper error handling in reflection

        addTestFromFile("Class Not Found Error",
                        errorPath + "classNotFound.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("For Name Invalid Parameterized",
                        errorPath + "forNameInvalidParameterized.mt",
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
        addTestFromFile("Invoke With Wrong Arity Error",
                        errorPath + "invokeWithWrongArity.mt",
                        TestType::ERROR_EXPECTED,
                        "cannot find method");

        // === EDGE CASE TESTS - private fields / inherited methods / null forName ===
        addOutputVerificationTest("Reflect Private Field",
                        passPath + "reflectPrivateField.mt");
        addOutputVerificationTest("Reflect Inherited Methods",
                        passPath + "reflectInheritedMethods.mt");

        addTestFromFile("Reflect ForName Null Error",
                        errorPath + "reflectForNameNull.mt",
                        TestType::ERROR_EXPECTED);
    }
}
