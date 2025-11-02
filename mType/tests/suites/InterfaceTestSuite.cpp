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
        // Advanced Generic Interface Tests
        // ====================================
        addOutputVerificationTest("Nested Generic Interface",
                        passPath + "interfaceNestedGeneric.mt");
        addOutputVerificationTest("Generic Interface With Bounds",
                        passPath + "interfaceGenericBounds.mt");
        addOutputVerificationTest("Generic Interface Covariance",
                        passPath + "interfaceGenericCovariance.mt");
        addOutputVerificationTest("Generic Interface Multiple Parameters",
                        passPath + "interfaceGenericMultipleParams.mt");
        addOutputVerificationTest("Generic Interface Inheritance Chain",
                        passPath + "interfaceGenericInheritanceChain.mt");
        addOutputVerificationTest("Generic Interface With Wildcards",
                        passPath + "interfaceGenericWildcards.mt");
        addOutputVerificationTest("Generic Interface Raw Type",
                        passPath + "interfaceGenericRawType.mt");

        // ====================================
        // Multiple Inheritance Tests
        // ====================================
        addOutputVerificationTest("Interface Multiple Inheritance",
                        passPath + "interfaceMultipleInheritance.mt");
        addOutputVerificationTest("Interface Diamond Inheritance",
                        passPath + "interfaceDiamondInheritance.mt");
        addOutputVerificationTest("Interface Deep Multiple Inheritance",
                        passPath + "interfaceDeepMultipleInheritance.mt");
        addOutputVerificationTest("Interface Conflicting Default Methods",
                        passPath + "interfaceConflictingDefaults.mt");
        addOutputVerificationTest("Interface Method Resolution",
                        passPath + "interfaceMethodResolution.mt");

        // ====================================
        // Static & Default Method Tests
        // ====================================
        addOutputVerificationTest("Interface Static Methods",
                        passPath + "interfaceStaticMethods.mt");
        addOutputVerificationTest("Interface Default Methods",
                        passPath + "interfaceDefaultMethods.mt");
        addOutputVerificationTest("Interface Default Override",
                        passPath + "interfaceDefaultOverride.mt");
        addOutputVerificationTest("Interface Static Call",
                        passPath + "interfaceStaticCall.mt");
        addOutputVerificationTest("Interface Mixed Methods",
                        passPath + "interfaceMixedMethods.mt");

        // ====================================
        // Type Compatibility Tests
        // ====================================
        addOutputVerificationTest("Interface Type Casting",
                        passPath + "interfaceTypeCasting.mt");
        addOutputVerificationTest("Interface Instanceof Check",
                        passPath + "interfaceInstanceof.mt");
        addOutputVerificationTest("Interface Type Hierarchy",
                        passPath + "interfaceTypeHierarchy.mt");
        addOutputVerificationTest("Interface Polymorphism",
                        passPath + "interfacePolymorphism.mt");
        addOutputVerificationTest("Interface Array Type",
                        passPath + "interfaceArrayType.mt");

        // ====================================
        // Lambda & Functional Interface Tests
        // ====================================
        addOutputVerificationTest("Functional Interface Lambda",
                        passPath + "interfaceFunctionalLambda.mt");
        addOutputVerificationTest("Generic Functional Interface",
                        passPath + "interfaceGenericFunctional.mt");
        addOutputVerificationTest("Functional Interface Method Reference",
                        passPath + "interfaceMethodReference.mt");
        addOutputVerificationTest("Functional Interface Composition",
                        passPath + "interfaceFunctionalComposition.mt");
        addOutputVerificationTest("Nested Lambda Interface",
                        passPath + "interfaceNestedLambda.mt");

        // ====================================
        // Advanced Implementation Tests
        // ====================================
        addOutputVerificationTest("Interface Partial Implementation",
                        passPath + "interfacePartialImplementation.mt");
        addOutputVerificationTest("Interface Abstract Class Bridge",
                        passPath + "interfaceAbstractBridge.mt");
        addOutputVerificationTest("Interface With Constructor",
                        passPath + "interfaceWithConstructor.mt");
        addOutputVerificationTest("Interface With Fields",
                        passPath + "interfaceWithFields.mt");
        addOutputVerificationTest("Interface Nested Types",
                        passPath + "interfaceNestedTypes.mt");

        // ====================================
        // Advanced Error Tests - Generics
        // ====================================
        addTestFromFile("Interface Generic Bound Violation Error",
                        errorPath + "interfaceGenericBoundViolation.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Interface Generic Arity Mismatch Error",
                        errorPath + "interfaceGenericArityMismatch.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Interface Generic Type Erasure Error",
                        errorPath + "interfaceGenericTypeErasure.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Interface Generic Wildcard Error",
                        errorPath + "interfaceGenericWildcardError.mt",
                        TestType::ERROR_EXPECTED);

        // ====================================
        // Advanced Error Tests - Inheritance
        // ====================================
        addTestFromFile("Interface Self Inheritance Error",
                        errorPath + "interfaceSelfInheritance.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Interface Duplicate Extends Error",
                        errorPath + "interfaceDuplicateExtends.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Interface Incompatible Inheritance Error",
                        errorPath + "interfaceIncompatibleInheritance.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Interface Circular Generic Error",
                        errorPath + "interfaceCircularGeneric.mt",
                        TestType::ERROR_EXPECTED);

        // ====================================
        // Advanced Error Tests - Implementation
        // ====================================
        addTestFromFile("Interface Method Signature Mismatch Error",
                        errorPath + "interfaceMethodSignatureMismatch.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Interface Visibility Mismatch Error",
                        errorPath + "interfaceVisibilityMismatch.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Interface Covariant Return Error",
                        errorPath + "interfaceCovariantReturnError.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Interface Multiple Implementation Error",
                        errorPath + "interfaceMultipleImplementationError.mt",
                        TestType::ERROR_EXPECTED);

        // ====================================
        // Advanced Error Tests - Lambda
        // ====================================
        addTestFromFile("Interface Non Functional Lambda Error",
                        errorPath + "interfaceNonFunctionalLambda.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Interface Lambda Signature Error",
                        errorPath + "interfaceLambdaSignature.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Interface Multiple Abstract Methods Error",
                        errorPath + "interfaceMultipleAbstractMethods.mt",
                        TestType::ERROR_EXPECTED);

        // ====================================
        // Advanced Error Tests - Type Safety
        // ====================================
        addTestFromFile("Interface Type Cast Error",
                        errorPath + "interfaceTypeCastError.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Interface Null Assignment Error",
                        errorPath + "interfaceNullAssignment.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Interface Array Covariance Error",
                        errorPath + "interfaceArrayCovarianceError.mt",
                        TestType::ERROR_EXPECTED);
    }
}
