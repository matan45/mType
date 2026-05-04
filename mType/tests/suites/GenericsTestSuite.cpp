#include "GenericsTestSuite.hpp"

namespace tests::testSuite
{
    using namespace testFramework;

    void GenericsTestSuite::setupTests()
    {
        // Basic generic functionality tests
        addOutputVerificationTest("Basic Generic Class",
                        passPath + "basicGenericClass.mt");
        addOutputVerificationTest("castToTypeParam",
                        passPath + "castToTypeParamThrows.mt");
        addOutputVerificationTest("three Type Arg",
                       passPath + "threeTypeArgDropsArg.mt");
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
        addOutputVerificationTest("HashMap Shape Specialization",
                        passPath + "hashmapShapeSpecialization.mt");
        addOutputVerificationTest("Generic Nullable Return Substitution",
                        passPath + "genericNullableReturnSubstitution.mt");

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
        addOutputVerificationTest("Static Generic Functional Interface Unify",
                        passPath + "staticGenericFunctionalInterfaceUnify.mt");

        // Instance generic method tests
        addOutputVerificationTest("Instance Generic Basic",
                        passPath + "instanceGenericBasic.mt");
        addOutputVerificationTest("Instance Generic Multiple Parameters",
                        passPath + "instanceGenericMultipleParams.mt");
        addOutputVerificationTest("Instance Generic Mixed Class Method",
                        passPath + "instanceGenericMixedClassMethod.mt");
        addOutputVerificationTest("Instance Generic Overloading",
                        passPath + "instanceGenericOverloading.mt");
        addOutputVerificationTest("Instance Generic Inheritance",
                        passPath + "instanceGenericInheritance.mt");
        addOutputVerificationTest("Instance Generic Null",
                        passPath + "instanceGenericNull.mt");
        addOutputVerificationTest("Instance Generic Complex Types",
                        passPath + "instanceGenericComplexTypes.mt");

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

        // === TYPE INFERENCE TESTS ===
        // Tests for generic type inference

        addOutputVerificationTest("Infer Diamond",
                        passPath + "inferDiamond.mt");
        addOutputVerificationTest("Infer Method Call",
                        passPath + "inferMethodCall.mt");
        addOutputVerificationTest("Infer Return Type",
                        passPath + "inferReturnType.mt");
        addOutputVerificationTest("Infer Lambda",
                        passPath + "inferLambda.mt");
        addOutputVerificationTest("Infer Contextual",
                        passPath + "inferContextual.mt");
        addOutputVerificationTest("Infer With Constraints",
                        passPath + "inferWithConstraints.mt");
        addOutputVerificationTest("Infer Nested Generics",
                        passPath + "inferNestedGenerics.mt");
        addOutputVerificationTest("Infer Array Type",
                        passPath + "inferArrayType.mt");
        addOutputVerificationTest("Infer Multiple Params",
                        passPath + "inferMultipleParams.mt");
        addOutputVerificationTest("Infer Recursive",
                        passPath + "inferRecursive.mt");

        // === ADVANCED TYPE INFERENCE TESTS (Phase 2 & 3) ===
        // Tests for nested generic inference and return type inference

        addOutputVerificationTest("Nested Generic Inference",
                        passPath + "nestedGenericInference.mt");
        addOutputVerificationTest("Deep Nested Generic Inference",
                        passPath + "deepNestedGenericInference.mt");
        addOutputVerificationTest("Return Type Inference",
                        passPath + "returnTypeInference.mt");
        addOutputVerificationTest("Partial Inference Combined",
                        passPath + "partialInferenceCombined.mt");

        // === VARIANCE TESTS ===
        // Tests for covariance and contravariance

        addOutputVerificationTest("Variance Covariant",
                        passPath + "varianceCovariant.mt");
        addOutputVerificationTest("Variance Contravariant",
                        passPath + "varianceContravariant.mt");
        addOutputVerificationTest("Variance Use Site",
                        passPath + "varianceUseSite.mt");
        addOutputVerificationTest("Variance Declaration Site",
                        passPath + "varianceDeclarationSite.mt");
        addOutputVerificationTest("Variance Wildcard Upper",
                        passPath + "varianceWildcardUpper.mt");
        addOutputVerificationTest("Variance Wildcard Lower",
                        passPath + "varianceWildcardLower.mt");
        addOutputVerificationTest("Variance Wildcard Unbounded",
                        passPath + "varianceWildcardUnbounded.mt");
        addOutputVerificationTest("Variance Method Params",
                        passPath + "varianceMethodParams.mt");
        addOutputVerificationTest("Variance Return Types",
                        passPath + "varianceReturnTypes.mt");
        addOutputVerificationTest("Variance Safe Casting",
                        passPath + "varianceSafeCasting.mt");

        // === TYPE ERASURE TESTS ===
        // Tests for type erasure and runtime behavior

        addOutputVerificationTest("Erasure Runtime Check",
                        passPath + "erasureRuntimeCheck.mt");
        addOutputVerificationTest("Erasure Instance Of",
                        passPath + "erasureInstanceOf.mt");
        addOutputVerificationTest("Erasure Type Token",
                        passPath + "erasureTypeToken.mt");
        addOutputVerificationTest("Erasure Reified",
                        passPath + "erasureReified.mt");
        addOutputVerificationTest("Erasure Bridge Method",
                        passPath + "erasureBridgeMethod.mt");
        addOutputVerificationTest("Erasure Arrays",
                        passPath + "erasureArrays.mt");
        addOutputVerificationTest("Erasure Reflection",
                        passPath + "erasureReflection.mt");
        addOutputVerificationTest("Erasure Unchecked",
                        passPath + "erasureUnchecked.mt");

        // === GENERIC LAMBDAS TESTS ===
        // Tests for lambdas with generic types

        addOutputVerificationTest("Lambda Functional Generic",
                        passPath + "lambdaFunctionalGeneric.mt");
        addOutputVerificationTest("Lambda Capture Generic",
                        passPath + "lambdaCaptureGeneric.mt");
        addOutputVerificationTest("Lambda Method Reference",
                        passPath + "lambdaMethodReference.mt");
        addOutputVerificationTest("Lambda Composition",
                        passPath + "lambdaComposition.mt");
        addOutputVerificationTest("Lambda Bounded Types",
                        passPath + "lambdaBoundedTypes.mt");
        addOutputVerificationTest("Lambda Inference",
                        passPath + "lambdaInference.mt");
        addOutputVerificationTest("Lambda Callback",
                        passPath + "lambdaCallback.mt");
        addOutputVerificationTest("Lambda Higher Order",
                        passPath + "lambdaHigherOrder.mt");

        // === ADVANCED CONSTRAINTS TESTS ===
        // Tests for complex constraint scenarios

        addOutputVerificationTest("Constraint Intersection",
                        passPath + "constraintIntersection.mt");
        addOutputVerificationTest("Constraint Multiple Interfaces",
                        passPath + "constraintMultipleInterfaces.mt");
        addOutputVerificationTest("Constraint Transitive",
                        passPath + "constraintTransitive.mt");
        addOutputVerificationTest("Constraint Self Ref Complex",
                        passPath + "constraintSelfRefComplex.mt");
        addOutputVerificationTest("Constraint Recursive Bounds",
                        passPath + "constraintRecursiveBounds.mt");
        addOutputVerificationTest("Constraint Propagation",
                        passPath + "constraintPropagation.mt");
        addOutputVerificationTest("Constraint Compound",
                        passPath + "constraintCompound.mt");
        addOutputVerificationTest("Constraint Wildcard",
                        passPath + "constraintWildcard.mt");

        // === GENERIC METHODS ADVANCED TESTS ===
        // Tests for advanced generic method scenarios

        addOutputVerificationTest("Method Ambiguity Resolution",
                        passPath + "methodAmbiguityResolution.mt");
        addOutputVerificationTest("Method Override Bounds",
                        passPath + "methodOverrideBounds.mt");
        addOutputVerificationTest("Method Covariant Generic",
                        passPath + "methodCovariantGeneric.mt");
        addOutputVerificationTest("Method Hiding",
                        passPath + "methodHiding.mt");
        addOutputVerificationTest("Method Static Shadowing",
                        passPath + "methodStaticShadowing.mt");
        addOutputVerificationTest("Method Raw Types",
                        passPath + "methodRawTypes.mt");
        addOutputVerificationTest("Method Reflection",
                        passPath + "methodReflection.mt");
        addOutputVerificationTest("Method Inlining",
                        passPath + "methodInlining.mt");

        // === INTEGRATION FEATURES TESTS ===
        // Tests for integration with other language features

        addOutputVerificationTest("Generic Exception",
                        passPath + "genericException.mt");
        addOutputVerificationTest("Generic Async",
                        passPath + "genericAsync.mt");
        addOutputVerificationTest("Generic Null Safety",
                        passPath + "genericNullSafety.mt");
        addOutputVerificationTest("Generic Serialization",
                        passPath + "genericSerialization.mt");

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

        // Instance generic method error tests
        addTestFromFile("Instance Generic Shadowing",
                    errorPath + "instanceGenericShadowing.mt",
                    TestType::ERROR_EXPECTED);
        addTestFromFile("Instance Generic Wrong Type Arg Count",
                    errorPath + "instanceGenericWrongTypeArgCount.mt",
                    TestType::ERROR_EXPECTED);
        addTestFromFile("Instance Generic Missing Type Args",
                    errorPath + "instanceGenericMissingTypeArgs.mt",
                    TestType::ERROR_EXPECTED);
        addTestFromFile("Instance Generic Undeclared Type Parameter",
                    errorPath + "instanceGenericUndeclaredTypeParam.mt",
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
        addTestFromFile("Bounded Wildcards Error",
                    errorPath + "boundedWildcardsError.mt",
                    TestType::ERROR_EXPECTED);
        addTestFromFile("Unbounded Wildcard Error",
                    errorPath + "unboundedWildcardError.mt",
                    TestType::ERROR_EXPECTED);
        addTestFromFile("Generic Cache Collision",
                    errorPath + "genericCacheCollision.mt",
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

        // === TYPE INFERENCE ERROR TESTS ===
        addTestFromFile("Infer Failure",
                    errorPath + "inferFailure.mt",
                    TestType::ERROR_EXPECTED);
        addTestFromFile("Infer Ambiguous",
                    errorPath + "inferAmbiguous.mt",
                    TestType::ERROR_EXPECTED);

        // Advanced type inference error tests (Phase 2 & 3)
        addTestFromFile("Nested Inference Conflict",
                    errorPath + "nestedInferenceConflict.mt",
                    TestType::ERROR_EXPECTED);
        addTestFromFile("Return Type Inference Conflict",
                    errorPath + "returnTypeInferenceConflict.mt",
                    TestType::ERROR_EXPECTED);

        // === ADVANCED CONSTRAINTS ERROR TESTS ===
        addTestFromFile("Constraint Violation Edge",
                    errorPath + "constraintViolationEdge.mt",
                    TestType::ERROR_EXPECTED);
        addTestFromFile("Constraint Circular",
                    errorPath + "constraintCircular.mt",
                    TestType::ERROR_EXPECTED);

        // === MYT-42: obj.getClass() ===
        addOutputVerificationTest("Get Class From Instance",
                    passPath + "getClassFromInstance.mt");
        addOutputVerificationTest("Get Class From Generic Instance",
                    passPath + "getClassFromGenericInstance.mt");
        addOutputVerificationTest("Get Class Identity Vs ForName",
                    passPath + "getClassIdentityVsForName.mt");

        // === EDGE CASE TESTS - nesting / shadowing / CRTP / empty ===
        addOutputVerificationTest("Recursive List Of List",
                    passPath + "recursiveListOfList.mt");
        addOutputVerificationTest("Generic Param Shadowing",
                    passPath + "genericParamShadowing.mt");
        // TODO MYT-222: re-enable when (T)cast against generic type-param no longer throws.
        // addOutputVerificationTest("CRTP Self Referential",
        //             passPath + "crtpSelfReferential.mt");
        addOutputVerificationTest("Empty Generic Array",
                    passPath + "emptyGenericArray.mt");
    }
}