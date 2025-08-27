#include "ClassTestSuite.hpp"

namespace tests::testSuite
{
    using namespace testFramework;

    void ClassTestSuite::setupTests()
    {
        // Basic class functionality tests
        addTestFromFile("Basic Class Definition",
                        passPath + "basicClassDefinition.mt");
        addTestFromFile("Class with Data Members",
                        passPath + "classWithDataMembers.mt");
        addTestFromFile("Class with Methods",
                        passPath + "classWithMethods.mt");
        addTestFromFile("Multiple Constructors",
                        passPath + "multipleConstructors.mt");
        addTestFromFile("Default Constructor",
                        passPath + "defaultConstructor.mt");
        addTestFromFile("Constructor with Parameters",
                        passPath + "constructorWithParameters.mt");

        // Static members and methods tests
        addTestFromFile("Static Data Members",
                        passPath + "staticDataMembers.mt");
        addTestFromFile("Static Methods",
                        passPath + "staticMethods.mt");
        addTestFromFile("Static Final Members",
                        passPath + "staticFinalMembers.mt");
        addTestFromFile("Mixed Static and Instance",
                        passPath + "mixedStaticAndInstance.mt");

        // Member access and object lifecycle tests
        addTestFromFile("Member Access",
                        passPath + "memberAccess.mt");
        addTestFromFile("Method Calls",
                        passPath + "methodCalls.mt");
        addTestFromFile("Chained Member Access",
                        passPath + "chainedMemberAccess.mt");
        addTestFromFile("Member Assignment",
                        passPath + "memberAssignment.mt");
        addTestFromFile("Object Creation",
                        passPath + "objectCreation.mt");
        addTestFromFile("Null Handling",
                        passPath + "nullHandling.mt");
        addTestFromFile("Object Assignment",
                        passPath + "objectAssignment.mt");
        addTestFromFile("Multiple Objects",
                        passPath + "multipleObjects.mt");

        // Complex scenarios tests
        addTestFromFile("Class with All Features",
                        passPath + "classWithAllFeatures.mt");
        addTestFromFile("Class in Functions",
                        passPath + "classInFunctions.mt");
        addTestFromFile("Class as Parameter",
                        passPath + "classAsParameter.mt");
        addTestFromFile("Class as Return Type",
                        passPath + "classAsReturnType.mt");
        addTestFromFile("Returning This and Method Chaining",
                        passPath + "returningThisAndChaining.mt");
        addTestFromFile("Complex Example",
                        passPath + "complexExample.mt");

        // Object reference edge case tests
        addTestFromFile("Circular Object References",
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
