#include "InterfaceTestSuite.hpp"

namespace tests::testSuite
{
    using namespace testFramework;

    void InterfaceTestSuite::setupTests()
    {
        // Basic interface functionality tests
        addOutputVerificationTest("Basic Interface Definition",
                        passPath + "basicInterfaceDefinition.mt");

        // Basic interface functionality tests
        addOutputVerificationTest("Complex Interface Import Inheritance Generics",
                        passPath + "complexInterfaceImportInheritanceGenerics.mt");

        addOutputVerificationTest("Interface Inheritance",
                        passPath + "interfaceInheritance.mt");

        // Generic interface tests
        addOutputVerificationTest("Generic Interface",
                        passPath + "genericInterface.mt");

        addOutputVerificationTest("Generic Interface Inheritance",
                        passPath + "genericInterfaceInheritance.mt");

        addOutputVerificationTest("Generic Constraints",
                        passPath + "genericConstraints.mt");

        // Interface implementation tests
        addOutputVerificationTest("Class Implements Interface",
                        passPath + "classImplementsInterface.mt");

        addOutputVerificationTest("Multiple Interface Implementation",
                        passPath + "multipleInterfaceImplementation.mt");

        addOutputVerificationTest("Generic Class Implements Generic Interface",
                        passPath + "genericClassImplementsGenericInterface.mt");

        // Interface as type tests
        addOutputVerificationTest("Interface As Type",
                        passPath + "interfaceAsType.mt");

        // === FINAL INTERFACE TESTS ===
        // These tests verify final interface functionality

        addOutputVerificationTest("Final Interface Definition",
                        passPath + "finalInterfaceDefinition.mt");
        addOutputVerificationTest("Implement Final Interface",
                        passPath + "implementFinalInterface.mt");

        // Advanced interface tests - Edge cases
        addOutputVerificationTest("Deeply Nested Interface Inheritance",
                        passPath + "deeplyNestedInheritance.mt");

        addOutputVerificationTest("Multiple Interface Inheritance Chains",
                        passPath + "multipleInheritanceChains.mt");

        addOutputVerificationTest("Interface with Many Generic Parameters",
                        passPath + "manyGenericParameters.mt");

        // === NEW EDGE CASE TESTS ===
        // Advanced interface scenarios

        addOutputVerificationTest("Multiple Diamond Implementation",
                        passPath + "multipleDiamondImplementation.mt");
        addOutputVerificationTest("Interface Multiple Casting",
                        passPath + "interfaceMultipleCasting.mt");

        // Error tests (expected to fail)
        addTestFromFile("Invalid Interface Name Error",
                        errorPath + "invalidInterfaceName.mt",
                        TestType::ERROR_EXPECTED);

        addTestFromFile("Interface Instantiation Error",
                        errorPath + "interfaceInstantiation.mt",
                        TestType::ERROR_EXPECTED);

        addTestFromFile("Incomplete Interface Implementation Error",
                        errorPath + "incompleteInterfaceImplementation.mt",
                        TestType::ERROR_EXPECTED);

        addTestFromFile("Wrong Return Type Error",
                        errorPath + "wrongReturnType.mt",
                        TestType::ERROR_EXPECTED);

        addTestFromFile("Wrong Parameter Count Error",
                        errorPath + "wrongParameterCount.mt",
                        TestType::ERROR_EXPECTED);

        addTestFromFile("Wrong Parameter Type Error",
                        errorPath + "wrongParameterType.mt",
                        TestType::ERROR_EXPECTED);

        addTestFromFile("Missing Multiple Methods Error",
                        errorPath + "missingMultipleMethods.mt",
                        TestType::ERROR_EXPECTED);

        addTestFromFile("Undefined Interface Error",
                        errorPath + "undefinedInterface.mt",
                        TestType::ERROR_EXPECTED);

        // Interface compatibility error tests
        addTestFromFile("Parameter Type Mismatch Error",
                        errorPath + "parameterTypeMismatch.mt",
                        TestType::ERROR_EXPECTED);

        addTestFromFile("Return Type Mismatch Error",
                        errorPath + "returnTypeMismatch.mt",
                        TestType::ERROR_EXPECTED);

        addTestFromFile("Static Parameter Mismatch Error",
                        errorPath + "staticParameterMismatch.mt",
                        TestType::ERROR_EXPECTED);

        addTestFromFile("Static Method Parameter Mismatch Error",
                        errorPath + "staticMethodParameterMismatch.mt",
                        TestType::ERROR_EXPECTED);

        addTestFromFile("Static Return Mismatch Error",
                        errorPath + "staticReturnMismatch.mt",
                        TestType::ERROR_EXPECTED);

        addTestFromFile("Multiple Parameter Mismatch Error",
                        errorPath + "multipleParameterMismatch.mt",
                        TestType::ERROR_EXPECTED);

        addTestFromFile("Assignment Type Mismatch Error",
                        errorPath + "assignmentTypeMismatch.mt",
                        TestType::ERROR_EXPECTED);

        // Advanced error tests - Edge cases
        addTestFromFile("Circular Interface Inheritance Deep Error",
                        errorPath + "circularInheritanceDeep.mt",
                        TestType::ERROR_EXPECTED);

        addTestFromFile("Too Many Generic Parameters Error",
                        errorPath + "tooManyGenericParameters.mt",
                        TestType::ERROR_EXPECTED);

        addTestFromFile("Interface Extends Class Error",
                        errorPath + "interfaceExtendsClass.mt",
                        TestType::ERROR_EXPECTED);

        addTestFromFile("Extends Final Interface Error",
                        errorPath + "extendsFinalInterface.mt",
                        TestType::ERROR_EXPECTED);

        addTestFromFile("Static Method Not Allowed Error",
                        errorPath + "staticMethodNotAllowed.mt",
                        TestType::ERROR_EXPECTED);

        // === NEW EDGE CASE ERROR TESTS ===
        // Advanced interface error scenarios

        addTestFromFile("Diamond Method Conflict Error",
                        errorPath + "diamondMethodConflict.mt",
                        TestType::ERROR_EXPECTED);

        // ====================================
        // ADDITIONAL TESTS - Files that exist but weren't registered
        // ====================================

        // === GENERIC INTERFACE TESTS (13 tests) ===
        addOutputVerificationTest("Complex Generic Parameter Test",
                        passPath + "complexGenericParameterTest.mt");
        addOutputVerificationTest("Interface Bounded Wildcards",
                        passPath + "interfaceBoundedWildcards.mt");
        addOutputVerificationTest("Interface Deeply Nested Generics",
                        passPath + "interfaceDeeplyNestedGenerics.mt");
        addOutputVerificationTest("Interface Generic Array Type",
                        passPath + "interfaceGenericArrayType.mt");
        addOutputVerificationTest("Interface Generic Array Types",
                        passPath + "interfaceGenericArrayTypes.mt");
        addOutputVerificationTest("Interface Generic Collection Return",
                        passPath + "interfaceGenericCollectionReturn.mt");
        addOutputVerificationTest("Interface Generic Self Reference",
                        passPath + "interfaceGenericSelfReference.mt");
        addOutputVerificationTest("Interface Method Level Generics",
                        passPath + "interfaceMethodLevelGenerics.mt");
        addOutputVerificationTest("Interface Multiple Constraint Bounds",
                        passPath + "interfaceMultipleConstraintBounds.mt");
        addOutputVerificationTest("Interface Nested Generic Interfaces",
                        passPath + "interfaceNestedGenericInterfaces.mt");
        addOutputVerificationTest("Interface Partial Generic Substitution",
                        passPath + "interfacePartialGenericSubstitution.mt");
        addOutputVerificationTest("Interface Recursive Generic",
                        passPath + "interfaceRecursiveGeneric.mt");
        addOutputVerificationTest("Interface Type Erasure",
                        passPath + "interfaceTypeErasure.mt");

        // === LAMBDA & FUNCTIONAL INTERFACE TESTS (6 tests) ===
        addOutputVerificationTest("Interface Generic Functional Lambda",
                        passPath + "interfaceGenericFunctionalLambda.mt");
        addOutputVerificationTest("Interface Lambda Cast",
                        passPath + "interfaceLambdaCast.mt");
        addOutputVerificationTest("Interface Lambda Generic Inference",
                        passPath + "interfaceLambdaGenericInference.mt");
        addOutputVerificationTest("Interface Lambda Type Inference",
                        passPath + "interfaceLambdaTypeInference.mt");
        addOutputVerificationTest("Interface Method Reference Lambda",
                        passPath + "interfaceMethodReferenceLambda.mt");
        addOutputVerificationTest("Interface SAM Lambda",
                        passPath + "interfaceSAMLambda.mt");

        // === ADVANCED IMPLEMENTATION TESTS (7 tests) ===
        addOutputVerificationTest("Interface Abstract Partial Implementation",
                        passPath + "interfaceAbstractPartialImplementation.mt");
        addOutputVerificationTest("Interface Deep And Wide",
                        passPath + "interfaceDeepAndWide.mt");
        addOutputVerificationTest("Interface Implementation Overload",
                        passPath + "interfaceImplementationOverload.mt");
        addOutputVerificationTest("Interface Many Implementations",
                        passPath + "interfaceManyImplementations.mt");
        addOutputVerificationTest("Interface Multiple Overloads",
                        passPath + "interfaceMultipleOverloads.mt");
        addOutputVerificationTest("Interface Overloaded Methods",
                        passPath + "interfaceOverloadedMethods.mt");
        addOutputVerificationTest("Interface Overload Resolution",
                        passPath + "interfaceOverloadResolution.mt");

        // === TYPE COMPATIBILITY & CASTING TESTS (9 tests) ===
        addOutputVerificationTest("Interface Cast From Base",
                        passPath + "interfaceCastFromBase.mt");
        addOutputVerificationTest("Interface Collection Type",
                        passPath + "interfaceCollectionType.mt");
        addOutputVerificationTest("Interface Covariant Return Types",
                        passPath + "interfaceCovariantReturnTypes.mt");
        addOutputVerificationTest("Interface InstanceOf",
                        passPath + "interfaceInstanceOf.mt");
        addOutputVerificationTest("Interface Self Referential",
                        passPath + "interfaceSelfReferential.mt");
        addOutputVerificationTest("Interface Self Returning",
                        passPath + "interfaceSelfReturning.mt");
        addOutputVerificationTest("Interface Type Narrowing",
                        passPath + "interfaceTypeNarrowing.mt");
        addOutputVerificationTest("Interface Valid Crosscast",
                        passPath + "interfaceValidCrosscast.mt");
        addOutputVerificationTest("Interface Variance Covariant",
                        passPath + "interfaceVarianceCovariant.mt");

        // === NULL HANDLING TESTS (5 tests) ===
        addOutputVerificationTest("Interface Null Assignment",
                        passPath + "interfaceNullAssignment.mt");
        addOutputVerificationTest("Interface Null Check",
                        passPath + "interfaceNullCheck.mt");
        addOutputVerificationTest("Interface Null Parameter",
                        passPath + "interfaceNullParameter.mt");
        addOutputVerificationTest("Interface Null Return",
                        passPath + "interfaceNullReturn.mt");
        addOutputVerificationTest("Interface Nullable Type",
                        passPath + "interfaceNullableType.mt");

        // === DEPENDENCY TESTS (2 tests) ===
        addOutputVerificationTest("Interface Mutual Dependency",
                        passPath + "interfaceMutualDependency.mt");
        addOutputVerificationTest("Interface Very Wide",
                        passPath + "interfaceVeryWide.mt");

        // === ADDITIONAL ERROR TESTS (10 tests) ===
        addTestFromFile("Interface Cast To Class Error",
                        errorPath + "interfaceCastToClass.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Interface Class And Interface Collision Error",
                        errorPath + "interfaceClassAndInterfaceCollision.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Interface Conflicting Signatures Error",
                        errorPath + "interfaceConflictingSignatures.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Interface Generic Primitive Constraint Error",
                        errorPath + "interfaceGenericPrimitiveConstraint.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Interface Invalid Cast Error",
                        errorPath + "interfaceInvalidCast.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Interface Method Conflict Chain Error",
                        errorPath + "interfaceMethodConflictChain.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Interface Multiple Inheritance Conflict Error",
                        errorPath + "interfaceMultipleInheritanceConflict.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Interface Non Public Implementation Error",
                        errorPath + "interfaceNonPublicImplementation.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Interface Private Implementation Error",
                        errorPath + "interfacePrivateImplementation.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Interface Protected Implementation Error",
                        errorPath + "interfaceProtectedImplementation.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Interface Raw Generic Type Error",
                        errorPath + "interfaceRawGenericType.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Interface Variance Contravariant Error",
                        errorPath + "interfaceVarianceContravariant.mt",
                        TestType::ERROR_EXPECTED);
    }
}
