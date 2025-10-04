#include "ModifiersTestSuite.hpp"

namespace tests::testSuite
{
    using namespace testFramework;

    void ModifiersTestSuite::setupTests()
    {
        // ===== PUBLIC MODIFIER TESTS =====
        addOutputVerificationTest("Public Fields - Same Class Access",
                        passPath + "publicFieldsSameClass.mt");
        addOutputVerificationTest("Public Methods - Same Class Access",
                        passPath + "publicMethodsSameClass.mt");
        addOutputVerificationTest("Public Fields - External Access",
                        passPath + "publicFieldsExternal.mt");
        addOutputVerificationTest("Public Methods - External Access",
                        passPath + "publicMethodsExternal.mt");
        addOutputVerificationTest("Public Constructors",
                        passPath + "publicConstructors.mt");
        addOutputVerificationTest("Public Static Members",
                        passPath + "publicStaticMembers.mt");

        // ===== PRIVATE MODIFIER TESTS =====
        addOutputVerificationTest("Private Fields - Same Class Access",
                        passPath + "privateFieldsSameClass.mt");
        addOutputVerificationTest("Private Methods - Same Class Access",
                        passPath + "privateMethodsSameClass.mt");
        addOutputVerificationTest("Private Static Members - Same Class",
                        passPath + "privateStaticSameClass.mt");
        addOutputVerificationTest("Private Constructor - Factory Pattern",
                        passPath + "privateConstructorFactory.mt");

        // ===== PROTECTED MODIFIER TESTS =====
        addOutputVerificationTest("Protected Fields - Same Class Access",
                        passPath + "protectedFieldsSameClass.mt");
        addOutputVerificationTest("Protected Methods - Same Class Access",
                        passPath + "protectedMethodsSameClass.mt");
        addOutputVerificationTest("Protected Members - Subclass Access",
                        passPath + "protectedSubclassAccess.mt");
        addOutputVerificationTest("Protected Static - Inheritance",
                        passPath + "protectedStaticInheritance.mt");

        // ===== DEFAULT MODIFIER TESTS =====
        addOutputVerificationTest("Class Default Modifiers (Private)",
                        passPath + "classDefaultPrivate.mt");
        addOutputVerificationTest("Constructor Default Modifiers (Public)",
                        passPath + "constructorDefaultPublic.mt");
        addOutputVerificationTest("Interface Default Modifiers (Public)",
                        passPath + "interfaceDefaultPublic.mt");

        // ===== MIXED MODIFIER TESTS =====
        addOutputVerificationTest("Mixed Access Modifiers",
                        passPath + "mixedAccessModifiers.mt");
        addOutputVerificationTest("Getter/Setter Pattern",
                        passPath + "getterSetterPattern.mt");
        addOutputVerificationTest("Encapsulation Example",
                        passPath + "encapsulationExample.mt");

        // ===== ERROR TESTS - PRIVATE ACCESS VIOLATIONS =====
        addTestFromFile("Private Field - External Access Violation",
                        errorPath + "privateFieldExternal.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Private Method - External Access Violation",
                        errorPath + "privateMethodExternal.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Private Static Field - External Access",
                        errorPath + "privateStaticFieldExternal.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Private Static Method - External Access",
                        errorPath + "privateStaticMethodExternal.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Private Constructor - External Access",
                        errorPath + "privateConstructorExternal.mt",
                        TestType::ERROR_EXPECTED);

        // ===== ERROR TESTS - PROTECTED ACCESS VIOLATIONS =====
        addTestFromFile("Protected Field - External Access Violation",
                        errorPath + "protectedFieldExternal.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Protected Method - External Access Violation",
                        errorPath + "protectedMethodExternal.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Protected Constructor - External Access",
                        errorPath + "protectedConstructorExternal.mt",
                        TestType::ERROR_EXPECTED);

        // ===== ERROR TESTS - INTERFACE RESTRICTIONS =====
        addTestFromFile("Interface - Private Method Not Allowed",
                        errorPath + "interfacePrivateMethod.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Interface - Protected Method Not Allowed",
                        errorPath + "interfaceProtectedMethod.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Interface - Private Field Not Allowed",
                        errorPath + "interfacePrivateField.mt",
                        TestType::ERROR_EXPECTED);

        // ===== ERROR TESTS - INHERITANCE VIOLATIONS =====
        addTestFromFile("Protected Field - Non-Subclass Access",
                        errorPath + "protectedNonSubclass.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Protected Method - Non-Subclass Access",
                        errorPath + "protectedMethodNonSubclass.mt",
                        TestType::ERROR_EXPECTED);
    }
}
