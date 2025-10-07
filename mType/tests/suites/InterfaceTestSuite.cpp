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

        // Advanced interface tests - Edge cases
        addOutputVerificationTest("Deeply Nested Interface Inheritance",
                        passPath + "deeplyNestedInheritance.mt");

        addOutputVerificationTest("Multiple Interface Inheritance Chains",
                        passPath + "multipleInheritanceChains.mt");

        addOutputVerificationTest("Interface with Many Generic Parameters",
                        passPath + "manyGenericParameters.mt");

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
    }
}
