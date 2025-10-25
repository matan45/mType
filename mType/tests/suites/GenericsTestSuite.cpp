#include "GenericsTestSuite.hpp"

namespace tests::testSuite
{
    using namespace testFramework;

    void GenericsTestSuite::setupTests()
    {
        // Basic generic functionality tests
        addOutputVerificationTest("Basic Generic Class",
                        passPath + "basicGenericClass.mt");
        addOutputVerificationTest("Multiple Type Parameters",
                        passPath + "multipleTypeParameters.mt");
        addOutputVerificationTest("Generic with Primitive Types",
                        passPath + "genericWithPrimitiveTypes.mt");
        addOutputVerificationTest("Generic Constructors",
                        passPath + "genericConstructors.mt");
        addOutputVerificationTest("Generic Method Overloading",
                        passPath + "genericMethodOverloading.mt");
        addOutputVerificationTest("Generic Null Handling",
                        passPath + "genericNullHandling.mt");
        addOutputVerificationTest("Generic Import Test",
                        passPath + "genericImportTest.mt");
        addOutputVerificationTest("HashMap HashSet Optimization",
                        passPath + "hashmapHashsetOptimization.mt");

        // Stress tests for deeply nested generics
        addOutputVerificationTest("Deeply Nested Generics Stress Test",
                        passPath + "deeplyNestedGenericsStressTest.mt");
        addOutputVerificationTest("Generic Complexity Stress Test",
                        passPath + "genericComplexityStressTest.mt");

        // Static generic method tests
        addOutputVerificationTest("Static Generic Methods",
                        passPath + "staticGenericMethods.mt");
        addOutputVerificationTest("Static Generic Method Parameterless",
                        passPath + "staticGenericMethodParameterless.mt");
        addOutputVerificationTest("Mixed Static Generic Methods",
                        passPath + "mixedStaticGenericMethods.mt");
        addOutputVerificationTest("Static Generic Simple Returns",
                        passPath + "staticGenericSimpleReturns.mt");
        addOutputVerificationTest("Static Generic Simple Container",
                        passPath + "staticGenericSimpleContainer.mt");
        addOutputVerificationTest("Static Generic Collection Returns",
                        passPath + "staticGenericCollectionReturns.mt");

        // Global generic function tests
        addOutputVerificationTest("Global Generic Functions",
                        passPath + "globalGenericFunctions.mt");
        addOutputVerificationTest("Global Generic Multiple Parameters",
                        passPath + "globalGenericMultipleParams.mt");

        // Exception handling tests
        addOutputVerificationTest("Try-Catch-Finally with Return",
                        passPath + "tryCatchFinallyWithReturn.mt");

        // Generic interface constraint tests
        addOutputVerificationTest("Constrained Class Generics",
                        passPath + "constrainedClassGenerics.mt");
        addOutputVerificationTest("Constrained Static Method Generics",
                        passPath + "constrainedStaticMethodGenerics.mt");
        addOutputVerificationTest("Constrained Global Functions",
                        passPath + "constrainedGlobalFunctions.mt");
        addOutputVerificationTest("Multiple Constrained Parameters",
                        passPath + "multipleConstrainedParameters.mt");
        addOutputVerificationTest("Constraint with Inheritance",
                        passPath + "constraintWithInheritance.mt");
        addOutputVerificationTest("Nested Constraints",
                        passPath + "constraintNested.mt");

        // === EDGE CASE TESTS - NEW ===
        // Advanced generic scenarios and edge cases

        addOutputVerificationTest("Array of Generic Types",
                        passPath + "arrayOfGenericTypes.mt");
        addOutputVerificationTest("Self-Referencing Constraints",
                        passPath + "selfReferencingConstraint.mt");
        addOutputVerificationTest("Nested Generic Arrays",
                        passPath + "nestedGenericArrays.mt");
        addOutputVerificationTest("Generic Tree Structure",
                        passPath + "genericTreeStructure.mt");
        addOutputVerificationTest("Type Parameter Propagation",
                        passPath + "typeParameterPropagation.mt");
        addOutputVerificationTest("Generic Inheritance with Super Access",
                        passPath + "genericInheritanceSuper.mt");
        addOutputVerificationTest("Nested Generic Inheritance",
                        passPath + "nestedGenericInheritance.mt");

        // === PARAMETER TYPE VALIDATION TESTS ===
        // These tests verify correct handling of all parameter types

        addOutputVerificationTest("Parameter Types - Primitives",
                        passPath + "parameterTypesPrimitive.mt");
        addOutputVerificationTest("Parameter Types - Arrays",
                        passPath + "parameterTypesArrays.mt");
        addOutputVerificationTest("Parameter Types - Objects",
                        passPath + "parameterTypesObjects.mt");
        addOutputVerificationTest("Parameter Types - Generics with Extends",
                        passPath + "parameterTypesGenericsExtends.mt");
        addOutputVerificationTest("Parameter Types - Promise<T>",
                        passPath + "parameterTypesPromise.mt");
        addOutputVerificationTest("Parameter Types - Mixed Types",
                        passPath + "parameterTypesMixed.mt");

        // === RETURN TYPE VALIDATION TESTS ===
        // These tests verify correct handling of all return types

        addOutputVerificationTest("Return Types - Primitives",
                        passPath + "returnTypesPrimitive.mt");
        addOutputVerificationTest("Return Types - Arrays",
                        passPath + "returnTypesArrays.mt");
        addOutputVerificationTest("Return Types - Objects",
                        passPath + "returnTypesObjects.mt");
        addOutputVerificationTest("Return Types - Generics",
                        passPath + "returnTypesGenerics.mt");
        addOutputVerificationTest("Return Types - Promise<T>",
                        passPath + "returnTypesPromise.mt");

        // Error handling tests
        addTestFromFile("Invalid Type Argument Count",
                    errorPath + "invalidTypeArgumentCount.mt",
                    TestType::ERROR_EXPECTED);
        addTestFromFile("Non-Generic with Type Args",
                    errorPath + "nonGenericWithTypeArgs.mt",
                    TestType::ERROR_EXPECTED);
        addTestFromFile("Type Mismatch in Method",
                    errorPath + "typeMismatchInMethod.mt",
                    TestType::ERROR_EXPECTED);
        addTestFromFile("Undefined Generic Class",
                    errorPath + "undefinedGenericClass.mt",
                    TestType::ERROR_EXPECTED);
        addTestFromFile("Invalid Return Type Assignment",
                    errorPath + "invalidReturnTypeAssignment.mt",
                    TestType::ERROR_EXPECTED);

        // Static generic method error tests
        addTestFromFile("Static Generic Wrong Type Arg Count",
                    errorPath + "staticGenericWrongTypeArgCount.mt",
                    TestType::ERROR_EXPECTED);
        addTestFromFile("Static Non-Generic with Type Args",
                    errorPath + "staticNonGenericWithTypeArgs.mt",
                    TestType::ERROR_EXPECTED);

        // Primitive type validation error tests
        addTestFromFile("Primitive String Type Rejected",
                    errorPath + "primitiveStringType.mt",
                    TestType::ERROR_EXPECTED);
        addTestFromFile("Primitive Int Type Rejected",
                    errorPath + "primitiveIntType.mt",
                    TestType::ERROR_EXPECTED);
        addTestFromFile("Primitive Float Type Rejected",
                    errorPath + "primitiveFloatType.mt",
                    TestType::ERROR_EXPECTED);
        addTestFromFile("Primitive Bool Type Rejected",
                    errorPath + "primitiveBoolType.mt",
                    TestType::ERROR_EXPECTED);
        addTestFromFile("Primitive Void Type Rejected",
                    errorPath + "primitiveVoidType.mt",
                    TestType::ERROR_EXPECTED);
        addTestFromFile("Multiple Primitive Types Rejected",
                    errorPath + "multiplePrimitiveTypes.mt",
                    TestType::ERROR_EXPECTED);

        // Global generic function error tests
        addTestFromFile("Global Non-Generic with Type Args",
                    errorPath + "globalNonGenericWithTypeArgs.mt",
                    TestType::ERROR_EXPECTED);
        addTestFromFile("Global Generic Wrong Type Arg Count",
                    errorPath + "globalGenericWrongTypeArgCount.mt",
                    TestType::ERROR_EXPECTED);
        addTestFromFile("Global Generic Primitive Type",
                    errorPath + "globalGenericPrimitiveType.mt",
                    TestType::ERROR_EXPECTED);

        // Generic constraint violation error tests
        addTestFromFile("Constraint Violation - Wrong Class",
                    errorPath + "constraintViolationWrongClass.mt",
                    TestType::ERROR_EXPECTED);
        addTestFromFile("Constraint Multiple Interfaces",
                    errorPath + "constraintMultipleInterfaces.mt",
                    TestType::ERROR_EXPECTED);
        addTestFromFile("Constraint Static Method Violation",
                    errorPath + "constraintStaticMethodViolation.mt",
                    TestType::ERROR_EXPECTED);
        addTestFromFile("Constraint Global Function Violation",
                    errorPath + "constraintGlobalFunctionViolation.mt",
                    TestType::ERROR_EXPECTED);
        addTestFromFile("Constraint Wrong Interface",
                    errorPath + "constraintWrongInterface.mt",
                    TestType::ERROR_EXPECTED);

        // Parser validation error tests (compile-time errors)
        addTestFromFile("Too Many Generic Parameters - Class",
                    errorPath + "tooManyGenericParams.mt",
                    TestType::ERROR_EXPECTED);
        addTestFromFile("Too Many Generic Parameters - Interface",
                    errorPath + "tooManyGenericParamsInterface.mt",
                    TestType::ERROR_EXPECTED);
        addTestFromFile("Empty Generic Type Arguments",
                    errorPath + "emptyGenericType.mt",
                    TestType::ERROR_EXPECTED);
        addTestFromFile("Unmatched Generic Brackets",
                    errorPath + "unmatchedGenericBrackets.mt",
                    TestType::ERROR_EXPECTED);

        // === MALFORMED GENERIC TYPE STRING ERROR TESTS ===
        // These tests verify parser handles malformed generic syntax correctly

        addTestFromFile("Malformed Generic - Missing Close Bracket",
                    errorPath + "malformedGenericMissingClose.mt",
                    TestType::ERROR_EXPECTED);
        addTestFromFile("Malformed Generic - Nested Mismatch",
                    errorPath + "malformedGenericNestedMismatch.mt",
                    TestType::ERROR_EXPECTED);
        addTestFromFile("Malformed Generic - Invalid Characters",
                    errorPath + "malformedGenericInvalidChars.mt",
                    TestType::ERROR_EXPECTED);
        addTestFromFile("Malformed Generic - Numeric Literal",
                    errorPath + "malformedGenericNumeric.mt",
                    TestType::ERROR_EXPECTED);
        addTestFromFile("Malformed Generic - Multiple Commas",
                    errorPath + "malformedGenericMultipleCommas.mt",
                    TestType::ERROR_EXPECTED);
        addTestFromFile("Malformed Generic - Trailing Comma",
                    errorPath + "malformedGenericTrailingComma.mt",
                    TestType::ERROR_EXPECTED);
        addTestFromFile("Malformed Generic - Leading Comma",
                    errorPath + "malformedGenericLeadingComma.mt",
                    TestType::ERROR_EXPECTED);

        // Inheritance validation error tests (compile-time errors)
        addTestFromFile("Class Cannot Extend Interface",
                    errorPath + "classExtendsInterface.mt",
                    TestType::ERROR_EXPECTED);
        addTestFromFile("Class Cannot Extend Generic Interface",
                    errorPath + "classExtendsGenericInterface.mt",
                    TestType::ERROR_EXPECTED);
        addTestFromFile("Interface Cannot Extend Class",
                    errorPath + "interfaceExtendsClass.mt",
                    TestType::ERROR_EXPECTED);
        addTestFromFile("Interface Cannot Extend Generic Class",
                    errorPath + "interfaceExtendsGenericClass.mt",
                    TestType::ERROR_EXPECTED);

        // Final modifier validation error tests (compile-time errors)
        addTestFromFile("Cannot Extend Final Class",
                    errorPath + "extendFinalClass.mt",
                    TestType::ERROR_EXPECTED);
        addTestFromFile("Cannot Extend Final Generic Class",
                    errorPath + "extendFinalGenericClass.mt",
                    TestType::ERROR_EXPECTED);
        addTestFromFile("Cannot Extend Final Interface",
                    errorPath + "extendFinalInterface.mt",
                    TestType::ERROR_EXPECTED);
        addTestFromFile("Cannot Extend Final Generic Interface",
                    errorPath + "extendFinalGenericInterface.mt",
                    TestType::ERROR_EXPECTED);

        // Circular inheritance validation error tests (compile-time errors)
        addTestFromFile("Circular Class Inheritance - Simple",
                    errorPath + "circularClassInheritanceSimple.mt",
                    TestType::ERROR_EXPECTED);
        addTestFromFile("Circular Class Inheritance - Complex",
                    errorPath + "circularClassInheritanceComplex.mt",
                    TestType::ERROR_EXPECTED);
        addTestFromFile("Circular Class Inheritance - Self",
                    errorPath + "circularClassInheritanceSelf.mt",
                    TestType::ERROR_EXPECTED);
        addTestFromFile("Circular Interface Inheritance - Simple",
                    errorPath + "circularInterfaceInheritanceSimple.mt",
                    TestType::ERROR_EXPECTED);
        addTestFromFile("Circular Interface Inheritance - Complex",
                    errorPath + "circularInterfaceInheritanceComplex.mt",
                    TestType::ERROR_EXPECTED);
        addTestFromFile("Circular Interface Inheritance - Multiple Parents",
                    errorPath + "circularInterfaceInheritanceMultiple.mt",
                    TestType::ERROR_EXPECTED);

        // === NEW EDGE CASE ERROR TESTS ===
        // Advanced generic error scenarios

        addTestFromFile("Type Parameter Shadowing",
                    errorPath + "typeParameterShadowing.mt",
                    TestType::ERROR_EXPECTED);
        addTestFromFile("Recursive Type Bounds",
                    errorPath + "recursiveTypeBounds.mt",
                    TestType::ERROR_EXPECTED);
        addTestFromFile("Generic Array Type Mismatch",
                    errorPath + "genericArrayTypeMismatch.mt",
                    TestType::ERROR_EXPECTED);
        addTestFromFile("Incomplete Type Arguments",
                    errorPath + "incompleteTypeArguments.mt",
                    TestType::ERROR_EXPECTED);
        addTestFromFile("Circular Generic Dependency",
                    errorPath + "circularGenericDependency.mt",
                    TestType::ERROR_EXPECTED);
        addTestFromFile("Generic Cache Collision",
                    errorPath + "genericCacheCollision.mt",
                    TestType::ERROR_EXPECTED);
        addTestFromFile("Bounded Wildcards Error",
                    errorPath + "boundedWildcardsError.mt",
                    TestType::ERROR_EXPECTED);
        addTestFromFile("Unbounded Wildcard Error",
                    errorPath + "unboundedWildcardError.mt",
                    TestType::ERROR_EXPECTED);
        addTestFromFile("Contravariant Type Error",
                    errorPath + "contravariantTypeError.mt",
                    TestType::ERROR_EXPECTED);
        addTestFromFile("Multiple Constraints Error",
                    errorPath + "multipleConstraintsError.mt",
                    TestType::ERROR_EXPECTED);

        // === GENERIC INHERITANCE WITH SUPER ERROR TESTS ===
        // These tests verify that generic type parameters are correctly enforced with super

        addTestFromFile("Generic Inheritance Super Type Mismatch",
                    errorPath + "genericInheritanceSuperTypeMismatch.mt",
                    TestType::ERROR_EXPECTED);
    }
}