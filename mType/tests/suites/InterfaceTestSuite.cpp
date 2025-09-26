#include "InterfaceTestSuite.hpp"

namespace tests::testSuite
{
    using namespace testFramework;

    void InterfaceTestSuite::setupTests()
    {
        // Basic interface functionality tests
        addOutputVerificationTest("Basic Interface Definition",
                        passPath + "basicInterfaceDefinition.mt");

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
    }
}
