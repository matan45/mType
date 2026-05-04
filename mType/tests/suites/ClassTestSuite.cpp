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
        addOutputVerificationTest("Compound Member Assignment",
                        passPath + "compoundMemberAssignment.mt");
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

        // Exception handling tests
        addOutputVerificationTest("Try-Catch-Finally with Return",
                        passPath + "tryCatchFinallyWithReturn.mt");
        addOutputVerificationTest("Exception in Finally Block",
                        passPath + "exceptionInFinally.mt");

        // Object reference edge case tests
        addOutputVerificationTest("Circular Object References",
                        passPath + "circularObjectReferences.mt");

        // === INHERITANCE AND POLYMORPHISM TESTS ===
        // These tests verify class inheritance, method overriding, and polymorphism

        addOutputVerificationTest("Basic Inheritance",
                        passPath + "basicInheritance.mt");
        addOutputVerificationTest("Method Override",
                        passPath + "methodOverride.mt");
        addOutputVerificationTest("Constructor Chaining",
                        passPath + "constructorChaining.mt");
        addOutputVerificationTest("Polymorphism",
                        passPath + "polymorphism.mt");
        addOutputVerificationTest("Deep Inheritance",
                        passPath + "deepInheritance.mt");
        addOutputVerificationTest("Generic Inheritance",
                        passPath + "genericInheritance.mt");
        addOutputVerificationTest("Method Override and Polymorphism",
                        passPath + "methodOverridePolymorphism.mt");

        // === FINAL CLASS TESTS ===
        // These tests verify final class functionality

        addOutputVerificationTest("Final Class Definition",
                        passPath + "finalClassDefinition.mt");
        addOutputVerificationTest("Final Class With Members",
                        passPath + "finalClassWithMembers.mt");
        addOutputVerificationTest("Final Generic Class",
                        passPath + "finalGenericClass.mt");

        // === FINAL METHOD TESTS ===
        // These tests verify final method functionality

        addOutputVerificationTest("Final Method Basic",
                        passPath + "finalMethodBasic.mt");
        addOutputVerificationTest("Final Method Inheritance",
                        passPath + "finalMethodInheritance.mt");
        addOutputVerificationTest("Final Method With Non-Final",
                        passPath + "finalMethodWithNonFinal.mt");
        addOutputVerificationTest("Final Method Access Modifiers",
                        passPath + "finalMethodAccessModifiers.mt");

        // === ABSTRACT CLASS TESTS ===
        // These tests verify abstract class and method functionality

        addOutputVerificationTest("Abstract Class Basic",
                        passPath + "abstract_class_basic.mt");
        addOutputVerificationTest("Abstract Class Inheritance Chain",
                        passPath + "abstract_class_inheritance_chain.mt");
        addOutputVerificationTest("Abstract Class With Concrete Methods",
                        passPath + "abstract_class_with_concrete_methods.mt");
        addOutputVerificationTest("Abstract Class Multiple Abstract Methods",
                        passPath + "abstract_class_multiple_abstract_methods.mt");
        addOutputVerificationTest("Abstract Async Method",
                        passPath + "abstract_async_method.mt");
        addOutputVerificationTest("Abstract Method Correct Signature",
                        passPath + "abstract_correct_signature.mt");

        // === NEW EDGE CASE TESTS ===
        // Advanced OOP scenarios and edge cases

        addOutputVerificationTest("Covariant Return Types",
                        passPath + "covariantReturnTypes.mt");
        addOutputVerificationTest("Deep Inheritance 8 Levels",
                        passPath + "deepInheritance8Levels.mt");
        addOutputVerificationTest("Protected Access in Subclass",
                        passPath + "protectedAccessInSubclass.mt");

        // === SUPER FIELD ACCESS TESTS ===
        // These tests verify super.field read and write operations

        addOutputVerificationTest("Super Field Read and Write",
                        passPath + "superFieldReadWrite.mt");
        addOutputVerificationTest("Super Field Inheritance Chain",
                        passPath + "superFieldInheritanceChain.mt");
        addOutputVerificationTest("Super Field With Polymorphism",
                        passPath + "superFieldWithPolymorphism.mt");
        addOutputVerificationTest("Super Field Comprehensive",
                        passPath + "superFieldComprehensive.mt");
        // MYT-212: shadowed-field static binding for super.x and parent-typed receivers.
        addOutputVerificationTest("Super Field Static Binding",
                        passPath + "superFieldStaticBinding.mt");

        // === SUPER METHOD ACCESS TESTS ===
        // These tests verify super.method() calls with access modifiers

        addOutputVerificationTest("Super Method Access",
                        passPath + "superMethodAccess.mt");

        // === CONSTRUCTOR PATTERNS TESTS ===
        // Tests for various constructor patterns and delegation

        addOutputVerificationTest("Constructor Delegation",
                        passPath + "constructorDelegation.mt");
        addOutputVerificationTest("Constructor Private Singleton",
                        passPath + "constructorPrivateSingleton.mt");
        addOutputVerificationTest("Constructor Protected",
                        passPath + "constructorProtected.mt");
        addOutputVerificationTest("Constructor Exception",
                        passPath + "constructorException.mt");
        addOutputVerificationTest("Constructor Calling Order",
                        passPath + "constructorCallingOrder.mt");
        addOutputVerificationTest("Constructor Overload Complex",
                        passPath + "constructorOverloadComplex.mt");

        // === METHOD FEATURES TESTS ===
        // Tests for method overloading, covariance, and recursion

        addOutputVerificationTest("Method Overloading Basic",
                        passPath + "methodOverloadingBasic.mt");
        addOutputVerificationTest("Method Covariant Complex",
                        passPath + "methodCovariantComplex.mt");
        addOutputVerificationTest("Method Hiding Vs Overriding",
                        passPath + "methodHidingVsOverriding.mt");
        addOutputVerificationTest("Method Early Return",
                        passPath + "methodEarlyReturn.mt");
        addOutputVerificationTest("Method Recursive Simple",
                        passPath + "methodRecursiveSimple.mt");
        addOutputVerificationTest("Method Mutual Recursion",
                        passPath + "methodMutualRecursion.mt");
        addOutputVerificationTest("Method Chain Calls",
                        passPath + "methodChainCalls.mt");
        addOutputVerificationTest("Method Static Overload",
                        passPath + "methodStaticOverload.mt");

        // === FIELD MANAGEMENT TESTS ===
        // Tests for field initialization, shadowing, and lifecycle

        addOutputVerificationTest("Field Initialization Order",
                        passPath + "fieldInitializationOrder.mt");
        addOutputVerificationTest("Field Static Init Order",
                        passPath + "fieldStaticInitOrder.mt");
        addOutputVerificationTest("Field Shadowing",
                        passPath + "fieldShadowing.mt");
        addOutputVerificationTest("Field Uninitialized Defaults",
                        passPath + "fieldUninitializedDefaults.mt");
        addOutputVerificationTest("Void vs Null Returns",
                        passPath + "voidVsNullReturns.mt");
        addOutputVerificationTest("Uninitialized Slot Edge Cases",
                        passPath + "uninitializedSlotEdgeCases.mt");
        addOutputVerificationTest("Field Final Init Timing",
                        passPath + "fieldFinalInitTiming.mt");
        addOutputVerificationTest("Field Final Mutable Ref",
                        passPath + "fieldFinalMutableRef.mt");
        addOutputVerificationTest("Field Lazy Init",
                        passPath + "fieldLazyInit.mt");

        // === TYPE OPERATIONS TESTS ===
        // Tests for type casting, instanceof, and polymorphism

        addOutputVerificationTest("Type Upcast Explicit",
                        passPath + "typeUpcastExplicit.mt");
        addOutputVerificationTest("Type Downcast Validation",
                        passPath + "typeDowncastValidation.mt");
        addOutputVerificationTest("Type Instance Of",
                        passPath + "typeInstanceOf.mt");
        addOutputVerificationTest("Type Runtime Check",
                        passPath + "typeRuntimeCheck.mt");
        addOutputVerificationTest("Type Polymorphic Array",
                        passPath + "typePolymorphicArray.mt");
        addOutputVerificationTest("Type Null Safety",
                        passPath + "typeNullSafety.mt");
        addOutputVerificationTest("Type Array Covariance",
                        passPath + "typeArrayCovariance.mt");

        // === IMPLICIT OBJECT INHERITANCE TESTS ===
        // Tests for implicit Object base class (all classes inherit toString, equals, hashCode)

        addOutputVerificationTest("Implicit Object Inheritance",
                        passPath + "implicitObjectInheritance.mt");
        addOutputVerificationTest("Object As Type",
                        passPath + "objectAsType.mt");
        addOutputVerificationTest("Object Equals Null",
                        passPath + "objectEqualsNull.mt");
        addOutputVerificationTest("Value Class Object Methods",
                        passPath + "valueClassObjectMethods.mt");

        // === OBJECT LIFECYCLE TESTS ===
        // Tests for object methods and lifecycle management

        addOutputVerificationTest("Object Equals",
                        passPath + "objectEquals.mt");
        addOutputVerificationTest("Object Hash Code",
                        passPath + "objectHashCode.mt");
        addOutputVerificationTest("Object To String",
                        passPath + "objectToString.mt");
        addOutputVerificationTest("Object Clone",
                        passPath + "objectClone.mt");
        addOutputVerificationTest("Object Identity",
                        passPath + "objectIdentity.mt");
        addOutputVerificationTest("Object Resource Cleanup",
                        passPath + "objectResourceCleanup.mt");

        // === MYT-274: SYNTHESIZED hashCode / equals TESTS ===
        // Compiler-generated fast hashCode/equals replace the slow string-based
        // Object default for user classes that don't declare their own.
        addOutputVerificationTest("Synth hashCode Consistency",
                        passPath + "synthHashCodeConsistency.mt");
        addOutputVerificationTest("Synth equals Contract",
                        passPath + "synthEqualsContract.mt");
        addOutputVerificationTest("Synth equals Type Mismatch",
                        passPath + "synthEqualsTypeMismatch.mt");
        addOutputVerificationTest("Synth Inherited Fields",
                        passPath + "synthInheritedFields.mt");
        addOutputVerificationTest("Synth Value Class Untouched",
                        passPath + "synthValueClassUntouched.mt");
        addOutputVerificationTest("Synth Empty Class Uses Native",
                        passPath + "synthEmptyClassUsesNative.mt");
        addOutputVerificationTest("Synth HashMap Key",
                        passPath + "synthHashMapKey.mt");

        // === ADVANCED PATTERNS TESTS ===
        // Tests for advanced design patterns

        addOutputVerificationTest("Pattern Singleton",
                        passPath + "patternSingleton.mt");
        addOutputVerificationTest("Pattern Factory",
                        passPath + "patternFactory.mt");
        addOutputVerificationTest("Pattern Builder",
                        passPath + "patternBuilder.mt");
        addOutputVerificationTest("Pattern Immutable",
                        passPath + "patternImmutable.mt");

        // === LEXICAL SCOPING TESTS ===
        // These tests verify that mType implements proper lexical scoping

        // Method scoping tests
        addOutputVerificationTest("Method Scoping - Isolation",
                        passPath + "methodScopingPass.mt");

        // Static method and field scoping tests
        addOutputVerificationTest("Static Method and Field Scoping",
                        passPath + "staticScopingPass.mt");

        // Loop scoping tests
        addOutputVerificationTest("Loop Variable Scoping",
                        passPath + "loopScopingPass.mt");

        // Conditional scoping tests
        addOutputVerificationTest("Conditional Block Scoping",
                        passPath + "conditionalScopingPass.mt");

        // Function scoping tests
        addOutputVerificationTest("Function Lexical Scoping",
                        passPath + "functionScopingPass.mt");

        // Error tests (expected to fail)
        addTestFromFile("Synth Parent Declares Equals Child Does Not (MYT-274)",
                        errorPath + "synthParentDeclaresEqualsChildDoesNot.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Synth Grandparent Declares Equals Child Does Not (MYT-274)",
                        errorPath + "synthGrandparentDeclaresEqualsChildDoesNot.mt",
                        TestType::ERROR_EXPECTED);
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
        // NOTE: finalMethod.mt removed - final methods are now supported
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

        // === LEXICAL SCOPING ERROR TESTS ===
        // These tests verify that improper scoping is correctly rejected

        addTestFromFile("Method Cross-Scoping Error",
                        errorPath + "methodScopingFail.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Static Method Instance Access Error",
                        errorPath + "staticScopingFail.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Loop Variable Out-of-Scope Error",
                        errorPath + "loopScopingFail.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Conditional Block Out-of-Scope Error",
                        errorPath + "conditionalScopingFail.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Function Cross-Scoping Error",
                        errorPath + "functionScopingFail.mt",
                        TestType::ERROR_EXPECTED);

        // === INHERITANCE ERROR TESTS ===
        // These tests verify that inheritance errors are correctly detected

        addTestFromFile("Circular Inheritance Error",
                        errorPath + "circularInheritance.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Parent Not Found Error",
                        errorPath + "parentNotFound.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Invalid Override Error",
                        errorPath + "invalidOverride.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Missing Super Call Error",
                        errorPath + "missingSuper.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Invalid Super Placement Error",
                        errorPath + "invalidSuper.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Multiple Inheritance Error",
                        errorPath + "multipleInheritance.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("super syntax",
                        errorPath + "superSyntax.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Class Extends Interface Error",
                        errorPath + "classExtendsInterface.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Extends Final Class Error",
                        errorPath + "extendsFinalClass.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Extends Final Generic Class Error",
                        errorPath + "extendsFinalGenericClass.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("duplicate catch test",
                        errorPath + "duplicate_catch_test.mt",
                        TestType::ERROR_EXPECTED);

        // === ABSTRACT CLASS ERROR TESTS ===
        // These tests verify that abstract class errors are correctly detected

        addTestFromFile("Abstract Class Cannot Instantiate Error",
                        errorPath + "abstract_class_cannot_instantiate.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Abstract Method Not Implemented Error",
                        errorPath + "abstract_method_not_implemented.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Abstract And Final Conflict Error",
                        errorPath + "abstract_and_final_conflict.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Abstract Static Method Error",
                        errorPath + "abstract_static_method.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Abstract Method With Body Error",
                        errorPath + "abstract_method_with_body.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Abstract Method Wrong Signature Error",
                        errorPath + "abstract_wrong_signature.mt",
                        TestType::ERROR_EXPECTED);

        // === NEW EDGE CASE ERROR TESTS ===
        // Advanced OOP error scenarios

        addTestFromFile("Access Modifier Narrowing Error",
                        errorPath + "accessModifierNarrowing.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Static Calls Instance Error",
                        errorPath + "staticCallsInstanceError.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Private Field Child Access Error",
                        errorPath + "privateFieldChildAccess.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Abstract Method In Concrete Class Error",
                        errorPath + "abstractMethodInConcrete.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Abstract Generic Method Error",
                        errorPath + "abstractGenericMethod.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Static Method Hiding Error",
                        errorPath + "staticMethodHiding.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Final Method Override Error",
                        errorPath + "finalMethodOverrideError.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Final Abstract Conflict Error",
                        errorPath + "finalAbstractConflict.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Final Method Override Protected Error",
                        errorPath + "finalMethodOverrideProtected.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Final Method Override In Grandchild Error",
                        errorPath + "finalMethodOverrideInGrandchild.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Protected Interface Method Error",
                        errorPath + "protectedInterfaceMethod.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Super Constructor Wrong Args Error",
                        errorPath + "superConstructorWrongArgs.mt",
                        TestType::ERROR_EXPECTED);
        // Note: super.field when parent is Object resolves to own field (pre-existing runtime behavior)
        // superFieldNoParent is not an error case in the current runtime
        addTestFromFile("Super Field Private Access Error",
                        errorPath + "superFieldPrivate.mt",
                        TestType::ERROR_EXPECTED);
        // Note: super.field = x when parent is Object resolves to own field (pre-existing runtime behavior)
        // superFieldAssignNoParent is not an error case in the current runtime
        addTestFromFile("Super Method Private Access Error",
                        errorPath + "superMethodPrivate.mt",
                        TestType::ERROR_EXPECTED);

        // === SUPER IN STATIC CONTEXT ERROR TESTS ===
        // These tests verify that super cannot be used in static contexts

        addTestFromFile("Super In Static Method Error",
                        errorPath + "superInStaticMethod.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Super In Static Initializer Error",
                        errorPath + "superInStaticInitializer.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Super Static Method Call Error",
                        errorPath + "superStaticMethodCall.mt",
                        TestType::ERROR_EXPECTED);

        // === PRIVATE ACCESS VIA INHERITANCE ERROR TESTS ===
        // These tests verify that private members cannot be accessed from child classes

        addTestFromFile("Private Method Direct Access Error",
                        errorPath + "privateMethodDirectAccess.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Private Field Direct Access Error",
                        errorPath + "privateFieldDirectAccess.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Private Static Method Inheritance Error",
                        errorPath + "privateStaticMethodInheritance.mt",
                        TestType::ERROR_EXPECTED);

        // === VALUE CLASS TESTS ===
        // These tests verify value class functionality

        addOutputVerificationTest("Value Class Basic",
                        passPath + "valueClassBasic.mt");
        addOutputVerificationTest("Value Class Methods",
                        passPath + "valueClassMethods.mt");
        addOutputVerificationTest("Value Class Copy Semantics",
                        passPath + "valueClassCopySemantics.mt");
        addOutputVerificationTest("Value Class Structural Equality",
                        passPath + "valueClassEquality.mt");
        addOutputVerificationTest("Value Class Implements Interface",
                        passPath + "valueClassImplements.mt");
        addOutputVerificationTest("Value Class Nested",
                        passPath + "valueClassNested.mt");
        addOutputVerificationTest("Value Class Boxing Primitives",
                        passPath + "valueClassBoxing.mt");
        addOutputVerificationTest("Value Class JIT Performance",
                        passPath + "valueClassJitPerformance.mt");
        addOutputVerificationTest("Value Class JIT Field Access",
                        passPath + "valueClassJitFieldAccess.mt");
        addOutputVerificationTest("getFieldTypedNonThisAccess",
                        passPath + "getFieldTypedNonThisAccess.mt");
        addOutputVerificationTest("JIT Benchmark",
                        passPath + "jitBenchmark.mt");
        addOutputVerificationTest("JIT Float Arithmetic",
                        passPath + "jitFloatArithmetic.mt");
        addOutputVerificationTest("JIT Array Operations",
                        passPath + "jitArrayOps.mt");
        addOutputVerificationTest("JIT Object Operations",
                        passPath + "jitObjectOps.mt");
        addOutputVerificationTest("JIT Type Feedback Arithmetic",
                        passPath + "jitTypeFeedbackArithmetic.mt");
        addOutputVerificationTest("JIT IC Specialization",
                        passPath + "jitICSpecialization.mt");
        addOutputVerificationTest("JIT Nested Loop OSR",
                        passPath + "jitNestedLoop.mt");

        // === NESTED TYPE VALIDATION TESTS ===
        // These tests verify that nested classes and interfaces are properly rejected

        addTestFromFile("Nested Class in Class Error",
                        "mType/tests/testFiles/error/error/nestedClassInClass.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Nested Interface in Class Error",
                        "mType/tests/testFiles/error/error/nestedInterfaceInClass.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Nested Class in Interface Error",
                        "mType/tests/testFiles/error/error/nestedClassInInterface.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Nested Interface in Interface Error",
                        "mType/tests/testFiles/error/error/nestedInterfaceInInterface.mt",
                        TestType::ERROR_EXPECTED);

        // === CONSTRUCTOR PATTERNS ERROR TESTS ===
        addTestFromFile("Constructor Same Name Method",
                        errorPath + "constructorSameNameMethod.mt",
                        TestType::ERROR_EXPECTED);

        // === METHOD FEATURES ERROR TESTS ===
        addTestFromFile("Method Ambiguous Overload",
                        errorPath + "methodAmbiguousOverload.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Method Overload Return Type",
                        errorPath + "methodOverloadReturnType.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Method Missing Return Statement",
                        errorPath + "missingReturn.mt",
                        TestType::ERROR_EXPECTED);

        // === FIELD MANAGEMENT ERROR TESTS ===
        addTestFromFile("Field Circular Dependency (Static)",
                        errorPath + "fieldCircularDependency.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Field Circular Dependency (Instance)",
                        errorPath + "fieldCircularDependencyInstance.mt",
                        TestType::ERROR_EXPECTED);

        // === TYPE OPERATIONS ERROR TESTS ===
        addTestFromFile("Type Invalid Cast",
                        errorPath + "typeInvalidCast.mt",
                        TestType::ERROR_EXPECTED);

        // === VALUE CLASS ERROR TESTS ===
        // These tests verify that value class constraints are enforced

        addTestFromFile("Value Abstract Conflict Error",
                        errorPath + "value_abstract_conflict.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Value Class Extends Error",
                        errorPath + "value_extends_error.mt",
                        TestType::ERROR_EXPECTED);

        // === MYT-42: Object.getClass() polymorphism + error ===
        addOutputVerificationTest("Get Class Polymorphism",
                        passPath + "getClassPolymorphism.mt");
        addTestFromFile("Get Class On Null Error",
                        errorPath + "getClassOnNull.mt",
                        TestType::ERROR_EXPECTED);

        // === EDGE CASE TESTS - empty class / shadowing / static in abstract ===
        addOutputVerificationTest("Empty Class Definition",
                        passPath + "emptyClassDefinition.mt");
        addOutputVerificationTest("Field Shadowing Parent",
                        passPath + "fieldShadowingParent.mt");
        addOutputVerificationTest("Static Field In Abstract Class",
                        passPath + "staticFieldInAbstractClass.mt");
    }
}
