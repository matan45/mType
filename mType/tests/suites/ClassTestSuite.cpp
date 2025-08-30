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

        // Object reference edge case tests
        addOutputVerificationTest("Circular Object References",
                        passPath + "circularObjectReferences.mt");

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
    }
}
