#include "CastTestSuite.hpp"

namespace tests::testSuite
{
    using namespace testFramework;

    void CastTestSuite::setupTests()
    {
        // === PRIMITIVE CASTING TESTS (11 tests) ===
        addOutputVerificationTest("Int to Float Cast",
                        passPath + "intToFloatCast.mt");
        addOutputVerificationTest("Float to Int Cast",
                        passPath + "floatToIntCast.mt");
        addOutputVerificationTest("Int to Bool Cast",
                        passPath + "intToBoolCast.mt");
        addOutputVerificationTest("Bool to Int Cast",
                        passPath + "boolToIntCast.mt");
        addOutputVerificationTest("Int to String Cast",
                        passPath + "intToStringCast.mt");
        addOutputVerificationTest("Float to String Cast",
                        passPath + "floatToStringCast.mt");
        addOutputVerificationTest("Bool to String Cast",
                        passPath + "boolToStringCast.mt");
        addOutputVerificationTest("String to Bool Cast",
                        passPath + "stringToBoolCast.mt");
        addOutputVerificationTest("Chained Primitive Casts",
                        passPath + "chainedPrimitiveCasts.mt");
        addOutputVerificationTest("Same Type Cast",
                        passPath + "sameTypeCast.mt");
        addOutputVerificationTest("Primitive Cast in Expression",
                        passPath + "primitiveCastInExpression.mt");

        // === OBJECT CASTING TESTS (13 tests) ===
        addOutputVerificationTest("Upcast to Parent Class",
                        passPath + "upcastToParent.mt");
        addOutputVerificationTest("Downcast to Child Class",
                        passPath + "downcastToChild.mt");
        addOutputVerificationTest("Cast to Sibling Class",
                        passPath + "castToSibling.mt");
        addOutputVerificationTest("Multi-level Inheritance Cast",
                        passPath + "multiLevelCast.mt");
        addOutputVerificationTest("Cast with Method Override",
                        passPath + "castWithOverride.mt");
        addOutputVerificationTest("Cast and Access Members",
                        passPath + "castAndAccessMembers.mt");
        addOutputVerificationTest("Cast in Assignment",
                        passPath + "castInAssignment.mt");
        addOutputVerificationTest("Cast in Method Call",
                        passPath + "castInMethodCall.mt");
        addOutputVerificationTest("Cast Return Value",
                        passPath + "castReturnValue.mt");
        addOutputVerificationTest("Polymorphic Cast",
                        passPath + "polymorphicCast.mt");
        addOutputVerificationTest("Same Class Cast",
                        passPath + "sameClassCast.mt");
        addOutputVerificationTest("Deep Inheritance Cast",
                        passPath + "deepInheritanceCast.mt");
        addOutputVerificationTest("Cast with Constructor",
                        passPath + "castWithConstructor.mt");

        // === INTERFACE CASTING TESTS (7 tests) ===
        addOutputVerificationTest("Cast to Interface",
                        passPath + "castToInterface.mt");
        addOutputVerificationTest("Interface to Class Cast",
                        passPath + "interfaceToClassCast.mt");
        addOutputVerificationTest("Multiple Interfaces Cast",
                        passPath + "multipleInterfacesCast.mt");
        addOutputVerificationTest("Interface Inheritance Cast",
                        passPath + "interfaceInheritanceCast.mt");
        addOutputVerificationTest("Cast Interface Method Call",
                        passPath + "castInterfaceMethodCall.mt");
        addOutputVerificationTest("Class Implements Multiple Cast",
                        passPath + "classImplementsMultipleCast.mt");
        addOutputVerificationTest("Interface to Wrong Class Cast",
                        passPath + "interfaceToWrongClassCast.mt");

        // === GENERIC CASTING TESTS (7 tests) ===
        addOutputVerificationTest("Generic Class Cast",
                        passPath + "genericClassCast.mt");
        addOutputVerificationTest("Generic Type Parameter Cast",
                        passPath + "genericTypeParameterCast.mt");
        addOutputVerificationTest("Nested Generic Cast",
                        passPath + "nestedGenericCast.mt");
        addOutputVerificationTest("Generic Array Cast",
                        passPath + "genericArrayCast.mt");
        addOutputVerificationTest("Generic Inheritance Cast",
                        passPath + "genericInheritanceCast.mt");
        addOutputVerificationTest("Generic Interface Cast",
                        passPath + "genericInterfaceCast.mt");
        addOutputVerificationTest("Generic Wildcard Cast",
                        passPath + "genericWildcardCast.mt");

        // === COLLECTION CASTING TESTS (4 tests) ===
        addOutputVerificationTest("Array Element Cast",
                        passPath + "arrayElementCast.mt");
        addOutputVerificationTest("List Generic Cast",
                        passPath + "listGenericCast.mt");
        addOutputVerificationTest("Map Generic Cast",
                        passPath + "mapGenericCast.mt");
        addOutputVerificationTest("Collection with Interface Cast",
                        passPath + "collectionInterfaceCast.mt");

        // === NULL HANDLING TESTS (5 tests) ===
        addOutputVerificationTest("Null to Object Cast",
                        passPath + "nullToObjectCast.mt");
        addOutputVerificationTest("Null to Interface Cast",
                        passPath + "nullToInterfaceCast.mt");
        addOutputVerificationTest("Null Cast Assignment",
                        passPath + "nullCastAssignment.mt");
        addOutputVerificationTest("Null Cast in Conditional",
                        passPath + "nullCastInConditional.mt");
        addTestFromFile("Null to Primitive Cast Error",
                        errorPath + "nullToPrimitiveCast.mt",
                        TestType::ERROR_EXPECTED);

        // === isClassOf TESTS (9 tests) ===
        addOutputVerificationTest("isClassOf Primitive Types",
                        passPath + "isClassOfPrimitive.mt");
        addOutputVerificationTest("isClassOf Parent Class",
                        passPath + "isClassOfParent.mt");
        addOutputVerificationTest("isClassOf Child Class",
                        passPath + "isClassOfChild.mt");
        addOutputVerificationTest("isClassOf Interface",
                        passPath + "isClassOfInterface.mt");
        addOutputVerificationTest("isClassOf Multiple Interfaces",
                        passPath + "isClassOfMultipleInterfaces.mt");
        addOutputVerificationTest("isClassOf Null Value",
                        passPath + "isClassOfNull.mt");
        addOutputVerificationTest("isClassOf in Conditional",
                        passPath + "isClassOfConditional.mt");
        addOutputVerificationTest("isClassOf with Generic",
                        passPath + "isClassOfGeneric.mt");
        addOutputVerificationTest("isClassOf Deep Hierarchy",
                        passPath + "isClassOfDeepHierarchy.mt");

        // === INTEGRATION TESTS (5 tests) ===
        addOutputVerificationTest("Safe Downcast Pattern",
                        passPath + "safeDowncastPattern.mt");
        addOutputVerificationTest("Type Checking Before Cast",
                        passPath + "typeCheckBeforeCast.mt");
        addOutputVerificationTest("Polymorphic Collection Cast",
                        passPath + "polymorphicCollectionCast.mt");
        addOutputVerificationTest("Complex Type Hierarchy Cast",
                        passPath + "complexHierarchyCast.mt");
        addOutputVerificationTest("Cast with Namespace",
                        passPath + "castWithNamespace.mt");

        // === ERROR TESTS ===
        addTestFromFile("Invalid Primitive to Object Cast",
                        errorPath + "primitiveToObjectCast.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Invalid Object to Primitive Cast",
                        errorPath + "objectToPrimitiveCast.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Incompatible Class Cast",
                        errorPath + "incompatibleClassCast.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Invalid Interface Cast",
                        errorPath + "invalidInterfaceCast.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Generic Type Mismatch Cast",
                        errorPath + "genericTypeMismatchCast.mt",
                        TestType::ERROR_EXPECTED);
    }
}
