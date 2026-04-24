#include "IntegrationTestSuite.hpp"

namespace tests::testSuite
{
    using namespace testFramework;

    void IntegrationTestSuite::setupTests()
    {
        addOutputVerificationTest("Pure oop",
                                  passPath + "phase4_complete.mt");
        addOutputVerificationTest("Conversion basic",
                                  passPath + "test_type_conversion_basic.mt");
        addOutputVerificationTest("Generics simple",
                                  passPath + "test_simple_generics.mt");
        addOutputVerificationTest("Arrays simple",
                                  passPath + "test_array_types.mt");
        addOutputVerificationTest("Error Handling comprehensive",
                                  passPath + "test_error_handling.mt");
        // Complex Feature Integration tests
        addOutputVerificationTest("Classes with Final Variables and Loops",
                                  passPath + "classesWithFinalVariablesAndLoops.mt");
        addOutputVerificationTest("Nested Namespaces with Static Methods",
                                  passPath + "nestedNamespacesWithStaticMethods.mt");
        addOutputVerificationTest("Control Flow with Class Members and Namespaces",
                                  passPath + "controlFlowWithClassMembersAndNamespaces.mt");
        addOutputVerificationTest("Imports with Namespaces and Classes",
                                  passPath + "importsWithNamespacesAndClasses.mt");
        addOutputVerificationTest("Complex Inheritance with Namespaces",
                                  passPath + "complexInheritanceWithNamespaces.mt");
        addOutputVerificationTest("Final Variables Across Namespaces and Classes",
                                  passPath + "finalVariablesAcrossNamespacesAndClasses.mt");
        addOutputVerificationTest("Loops with Class Creation and Namespaces",
                                  passPath + "loopsWithClassCreationAndNamespaces.mt");
        addOutputVerificationTest("Recursive Namespace Class Interactions",
                                  passPath + "recursiveNamespaceClassInteractions.mt");
        addOutputVerificationTest("Concurrent Namespace and Class Modifications",
                                  passPath + "concurrentNamespaceAndClassModifications.mt");
        addOutputVerificationTest("Return Types with Namespaces",
                                  passPath + "returnWithNamespace.mt");

        // Edge Cases and Potential Bugs tests
        addOutputVerificationTest("Circular Namespace References",
                                  passPath + "circularNamespaceReferences.mt");
        addOutputVerificationTest("Namespace Class Name Collisions",
                                  passPath + "namespaceClassNameCollisions.mt");
        addOutputVerificationTest("Deep Nesting Limits",
                                  passPath + "deepNestingLimits.mt");
        addOutputVerificationTest("Memory Leaks with Complex Objects",
                                  passPath + "memoryLeaksWithComplexObjects.mt");
        addOutputVerificationTest("Large Data Structures",
                                  passPath + "largeDataStructures.mt");

        // Object Destruction Order and Cleanup Tests
        addOutputVerificationTest("Object Destruction Order Basic",
                                  passPath + "objectDestructionOrderBasic.mt");
        addOutputVerificationTest("Complex Object Cleanup Scenarios",
                                  passPath + "complexObjectCleanupScenarios.mt");
        addOutputVerificationTest("Object Lifecycle Edge Cases",
                                  passPath + "objectLifecycleEdgeCases.mt");
        addOutputVerificationTest("Memory Leak Prevention",
                                  passPath + "memoryLeakPrevention.mt");
        addOutputVerificationTest("Resource Cleanup Patterns",
                                  passPath + "resourceCleanupPatterns.mt");
        addOutputVerificationTest("Type System Edge Cases",
                                  passPath + "typeSystemEdgeCases.mt");

        // Performance and Stress tests
        addOutputVerificationTest("Performance with Large Namespaces",
                                  passPath + "performanceWithLargeNamespaces.mt");
        addOutputVerificationTest("Stress Test with Many Classes",
                                  passPath + "stressTestWithManyClasses.mt");
        addOutputVerificationTest("Complex Control Flow Performance",
                                  passPath + "complexControlFlowPerformance.mt");
        addOutputVerificationTest("Memory Usage with Deep Nesting",
                                  passPath + "memoryUsageWithDeepNesting.mt");

        // Error Handling and Recovery tests
        addOutputVerificationTest("Error Recovery with Partial Failures",
                                  passPath + "errorRecoveryWithPartialFailures.mt");
        addOutputVerificationTest("Invalid Combinations Handling",
                                  passPath + "invalidCombinationsHandling.mt");
        addOutputVerificationTest("Corrupted State Recovery",
                                  passPath + "corruptedStateRecovery.mt");

        // Exception Handling tests
        addOutputVerificationTest("Exception Inheritance Hierarchy",
                                  passPath + "exceptionInheritanceHierarchy.mt");
        addOutputVerificationTest("Exception in Finally Block",
                                  passPath + "exceptionInFinally.mt");
        addOutputVerificationTest("Exception Memory Lifecycle",
                                  passPath + "exceptionMemoryLifecycle.mt");

        addOutputVerificationTest("complex generic substitution",
                                  passPath + "test_type_substitution_edge_cases.mt");

        addOutputVerificationTest("Class Generic Serialization",
                                  passPath + "classGenericSerialization.mt");
        addOutputVerificationTest("Collections As Data Members",
                                  passPath + "collectionsAsDataMembers.mt");
        addOutputVerificationTest("Generic Class Compilation",
                                  passPath + "genericClassCompilation.mt");
        addOutputVerificationTest("Generic Import Serialization",
                                  passPath + "genericImportSerialization.mt");
        addOutputVerificationTest("Multi Type Generic Serialization",
                                  passPath + "multiTypeGenericSerialization.mt");
        addOutputVerificationTest("Static Generic Methods Serialization",
                                  passPath + "staticGenericMethodsSerialization.mt");
        addOutputVerificationTest("Switch Nested In Loops",
                                  passPath + "switchNestedInLoops.mt");
        addOutputVerificationTest("Test Import Main",
                                  passPath + "test_import_main.mt");

        // ====================================
        // NEW COMPREHENSIVE INTEGRATION TESTS
        // ====================================

        // Phase 1: Complex Feature Mix Tests
        addOutputVerificationTest("Test 01: Polymorphic Collections with Generics",
                                  passPath + "test01_polymorphicCollectionsWithGenerics.mt");
        addOutputVerificationTest("Test 02: Async Exception Handling with Inheritance",
                                  passPath + "test02_asyncExceptionHandlingWithInheritance.mt");
        addOutputVerificationTest("Test 03: Abstract Classes with Generic Constraints",
                                  passPath + "test03_abstractClassesWithGenericConstraints.mt");
        addOutputVerificationTest("Test 04: Lambda Closures with Auto-Boxing",
                                  passPath + "test04_lambdaClosuresWithAutoBoxing.mt");
        addOutputVerificationTest("Test 05: Imported Interfaces with Async Lambdas",
                                  passPath + "test05_importedInterfacesWithAsyncLambdas.mt");
        addOutputVerificationTest("Test 06: Complex Control Flow with Exceptions",
                                  passPath + "test06_complexControlFlowWithExceptions.mt");
        addOutputVerificationTest("Test 07: Polymorphic Calls with Final Classes",
                                  passPath + "test07_polymorphicCallsWithFinalClasses.mt");
        addOutputVerificationTest("Test 08: Multi-Dimensional Arrays with Generics",
                                  passPath + "test08_multiDimensionalArraysWithGenerics.mt");

        // Phase 2: Edge Case Tests
        addOutputVerificationTest("Test 09: Nested Generic Constraints Edge Cases",
                                  passPath + "test09_nestedGenericConstraintsEdgeCases.mt");
        addOutputVerificationTest("Test 10: Exception Hierarchy with Finally",
                                  passPath + "test10_exceptionHierarchyWithFinally.mt");
        addOutputVerificationTest("Test 11: Scoping Edge Cases with Lambdas",
                                  passPath + "test11_scopingEdgeCasesWithLambdas.mt");
        addOutputVerificationTest("Test 12: Auto-Boxing with Operator Overloading",
                                  passPath + "test12_autoBoxingWithOperatorOverloading.mt");
        addOutputVerificationTest("Test 13: Async/Await with Nested Try-Catch",
                                  passPath + "test13_asyncAwaitWithNestedTryCatch.mt");
        addOutputVerificationTest("Test 14: Interface Multiple Implementation Edge Cases",
                                  passPath + "test14_interfaceMultipleImplementationEdgeCases.mt");

        // Phase 3: Real-World Scenario Tests
        addOutputVerificationTest("Test 15: Repository Pattern with Generics",
                                  passPath + "test15_repositoryPatternWithGenerics.mt");
        addOutputVerificationTest("Test 16: Observer Pattern with Lambdas",
                                  passPath + "test16_observerPatternWithLambdas.mt");
        addOutputVerificationTest("Test 17: Builder Pattern with Method Chaining",
                                  passPath + "test17_builderPatternWithMethodChaining.mt");
        addOutputVerificationTest("Test 18: Strategy Pattern with Async Operations",
                                  passPath + "test18_strategyPatternWithAsyncOperations.mt");
        addOutputVerificationTest("Test 19: Factory Pattern with Imports",
                                  passPath + "test19_factoryPatternWithImports.mt");
        addOutputVerificationTest("Test 20: Decorator Pattern with Inheritance",
                                  passPath + "test20_decoratorPatternWithInheritance.mt");


        addOutputVerificationTest("Circular Namespace References",
                                  passPath + "circularNamespaceReferences.mt");
        addOutputVerificationTest("Classes With Final Variables And Loops",
                                  passPath + "classesWithFinalVariablesAndLoops.mt");
        addOutputVerificationTest("Class Generic Serialization",
                                  passPath + "classGenericSerialization.mt");
        addOutputVerificationTest("Collections As Data Members",
                                  passPath + "collectionsAsDataMembers.mt");
        addOutputVerificationTest("Complex Control Flow Performance",
                                  passPath + "complexControlFlowPerformance.mt");
        addOutputVerificationTest("Complex Inheritance With Namespaces",
                                  passPath + "complexInheritanceWithNamespaces.mt");
        addOutputVerificationTest("Complex Object Cleanup Scenarios",
                                  passPath + "complexObjectCleanupScenarios.mt");
        addOutputVerificationTest("Concurrent Namespace And Class Modifications",
                                  passPath + "concurrentNamespaceAndClassModifications.mt");
        addOutputVerificationTest("Corrupted State Recovery",
                                  passPath + "corruptedStateRecovery.mt");
        addOutputVerificationTest("Deep Nesting Limits",
                                  passPath + "deepNestingLimits.mt");
        addOutputVerificationTest("Error Recovery With Partial Failures",
                                  passPath + "errorRecoveryWithPartialFailures.mt");
        addOutputVerificationTest("Exception In Finally",
                                  passPath + "exceptionInFinally.mt");
        addOutputVerificationTest("Exception Inheritance Hierarchy",
                                  passPath + "exceptionInheritanceHierarchy.mt");
        addOutputVerificationTest("Exception Memory Lifecycle",
                                  passPath + "exceptionMemoryLifecycle.mt");
        addOutputVerificationTest("Generic Import Serialization",
                                  passPath + "genericImportSerialization.mt");
        addOutputVerificationTest("Imports With Namespaces And Classes",
                                  passPath + "importsWithNamespacesAndClasses.mt");
        addOutputVerificationTest("Invalid Combinations Handling",
                                  passPath + "invalidCombinationsHandling.mt");
        addOutputVerificationTest("Large Data Structures",
                                  passPath + "largeDataStructures.mt");
        addOutputVerificationTest("Loops With Class Creation And Namespaces",
                                  passPath + "loopsWithClassCreationAndNamespaces.mt");
        addOutputVerificationTest("Memory Leak Prevention",
                                  passPath + "memoryLeakPrevention.mt");
        addOutputVerificationTest("Memory Leaks With Complex Objects",
                                  passPath + "memoryLeaksWithComplexObjects.mt");
        addOutputVerificationTest("Memory Usage With Deep Nesting",
                                  passPath + "memoryUsageWithDeepNesting.mt");
        addOutputVerificationTest("Multi Type Generic Serialization",
                                  passPath + "multiTypeGenericSerialization.mt");
        addOutputVerificationTest("Namespace Class Name Collisions",
                                  passPath + "namespaceClassNameCollisions.mt");
        addOutputVerificationTest("Nested Namespaces With Static Methods",
                                  passPath + "nestedNamespacesWithStaticMethods.mt");
        addOutputVerificationTest("Object Destruction Order Basic",
                                  passPath + "objectDestructionOrderBasic.mt");
        addOutputVerificationTest("Object Lifecycle Edge Cases",
                                  passPath + "objectLifecycleEdgeCases.mt");
        addOutputVerificationTest("Performance With Large Namespaces",
                                  passPath + "performanceWithLargeNamespaces.mt");
        addOutputVerificationTest("Recursive Namespace Class Interactions",
                                  passPath + "recursiveNamespaceClassInteractions.mt");
        addOutputVerificationTest("Resource Cleanup Patterns",
                                  passPath + "resourceCleanupPatterns.mt");
        addOutputVerificationTest("Return With Namespace",
                                  passPath + "returnWithNamespace.mt");
        addOutputVerificationTest("Static Generic Methods Serialization",
                                  passPath + "staticGenericMethodsSerialization.mt");
        addOutputVerificationTest("Stress Test With Many Classes",
                                  passPath + "stressTestWithManyClasses.mt");
        addOutputVerificationTest("Switch Nested In Loops",
                                  passPath + "switchNestedInLoops.mt");
        addOutputVerificationTest("Test Array Types",
                                  passPath + "test_array_types.mt");
        addOutputVerificationTest("Test Error Handling",
                                  passPath + "test_error_handling.mt");
        addOutputVerificationTest("Test Simple Generics",
                                  passPath + "test_simple_generics.mt");
        addOutputVerificationTest("Test Type Conversion Basic",
                                  passPath + "test_type_conversion_basic.mt");
        addOutputVerificationTest("Type System Edge Cases",
                                  passPath + "typeSystemEdgeCases.mt");

        // Lazy re-boxing optimization tests
        addOutputVerificationTest("Lazy Reboxing Chained Arithmetic",
                                  passPath + "lazyReboxingChainedArithmetic.mt");
        addOutputVerificationTest("Lazy Reboxing Escape Points",
                                  passPath + "lazyReboxingEscapePoints.mt");
        addOutputVerificationTest("Lazy Reboxing Mixed Operands",
                                  passPath + "lazyReboxingMixedOperands.mt");

        // MYT-163 Phase F-a: speculative JIT inlining tests.
        addOutputVerificationTest("Inline Basic MONO",
                                  passPath + "inlining/inline_basic.mt");
        addOutputVerificationTest("Inline Arithmetic Hot Loop",
                                  passPath + "inlining/inline_arithmetic.mt");
        addOutputVerificationTest("Inline Monomorphic Acceptance",
                                  passPath + "inlining/inline_monomorphic.mt");
        addOutputVerificationTest("Inline Self-Recursive Guard",
                                  passPath + "inlining/inline_recursive_guard.mt");
        // MYT-167 Phase F-e: value-object receivers inline for read-only methods;
        // write-containing callees still fall through.
        addOutputVerificationTest("Inline Value Object Read-Only",
                                  passPath + "inlining/inline_value_object_readonly.mt");
        addOutputVerificationTest("Inline Value Object Write Skip",
                                  passPath + "inlining/inline_value_object_write_skip.mt");

        // MYT-164 Phase F-b: internal jumps + nested inlining.
        addOutputVerificationTest("Inline With If/Else Branches",
                                  passPath + "inlining/inline_with_if.mt");
        addOutputVerificationTest("Inline With While Loop",
                                  passPath + "inlining/inline_with_loop.mt");
        addOutputVerificationTest("Inline Nested Depth-2",
                                  passPath + "inlining/inline_nested.mt");

        // MYT-165 Phase F-c: polymorphic inlining with chained shape guards.
        addOutputVerificationTest("Inline Polymorphic Chained Guards",
                                  passPath + "inlining/inline_poly.mt");
        addOutputVerificationTest("Inline MEGA Fallback",
                                  passPath + "inlining/inline_mega_fallback.mt");

        // MYT-168: regression guard for MONO->POLY IC transition in JIT'd code.
        addOutputVerificationTest("Inline MONO to POLY Transition",
                                  passPath + "inlining/inline_mono_to_poly.mt");

        // MYT-210: plain-CALL / CALL_FAST inlining.
        addOutputVerificationTest("Inline Function Basic",
                                  passPath + "inlining/inline_function_basic.mt");
        addOutputVerificationTest("Inline Function Chain Depth-2",
                                  passPath + "inlining/inline_function_chain.mt");
        addOutputVerificationTest("Inline Function Recursion Bailout",
                                  passPath + "inlining/inline_function_recursion_bailout.mt");
        addOutputVerificationTest("Inline Function Arg Types",
                                  passPath + "inlining/inline_function_arg_types.mt");
        addOutputVerificationTest("Inline Function Object Args",
                                  passPath + "inlining/inline_function_object_args.mt");

        // MYT-173: CALL_METHOD_CACHED promotion + sticky deopt.
        addOutputVerificationTest("CALL_METHOD_CACHED Monomorphic Promote",
                                  passPath + "ic/call_method_cached_mono.mt");
        addOutputVerificationTest("CALL_METHOD_CACHED Deopt Sticky",
                                  passPath + "ic/call_method_cached_deopt.mt");

        // MYT-194: GET_FIELD_CACHED / SET_FIELD_CACHED promotion + sticky deopt.
        addOutputVerificationTest("GET_FIELD_CACHED Monomorphic Promote",
                                  passPath + "ic/get_field_cached_mono.mt");
        addOutputVerificationTest("SET_FIELD_CACHED Monomorphic Promote",
                                  passPath + "ic/set_field_cached_mono.mt");
        addOutputVerificationTest("GET_FIELD_CACHED Deopt Sticky",
                                  passPath + "ic/get_field_cached_deopt.mt");
        addOutputVerificationTest("SET_FIELD_CACHED Deopt Sticky",
                                  passPath + "ic/set_field_cached_deopt.mt");

        // MYT-199: type-quickened LOAD_LOCAL / STORE_LOCAL. Four mono tests
        // drive each specialized variant through its fast path; sticky-deopt
        // is exercised indirectly by any pre-existing test whose locals hold
        // heterogeneous tags (e.g. nullable slots), which re-enter the
        // generic path after one miss.
        addOutputVerificationTest("LOAD_LOCAL_INT Monomorphic Promote",
                                  passPath + "ic/load_local_int_mono.mt");
        addOutputVerificationTest("LOAD_LOCAL_FLOAT Monomorphic Promote",
                                  passPath + "ic/load_local_float_mono.mt");
        addOutputVerificationTest("LOAD_LOCAL_BOOL Monomorphic Promote",
                                  passPath + "ic/load_local_bool_mono.mt");
        addOutputVerificationTest("LOAD_LOCAL_BOXED_INST Monomorphic Promote",
                                  passPath + "ic/load_local_boxed_mono.mt");

        // MYT-198: superinstruction fusion of adjacent CACHED / ADD_INT pairs.
        addOutputVerificationTest("LOAD_LOCAL_CALL_CACHED Fusion",
                                  passPath + "ic/load_local_call_cached_fuse.mt");
        addOutputVerificationTest("LOAD_LOCAL_GET_FIELD_CACHED Fusion",
                                  passPath + "ic/load_local_get_field_cached_fuse.mt");
        addOutputVerificationTest("ADD_INT_CONST Fusion",
                                  passPath + "ic/add_int_const_fuse.mt");
        addOutputVerificationTest("Fused Pair Deopt Unfuse",
                                  passPath + "ic/fused_deopt.mt");
        addOutputVerificationTest("Fusion Skipped In Lambda Frame",
                                  passPath + "ic/fused_lambda_safe.mt");
        addOutputVerificationTest("Fusion Skipped At Jump Target",
                                  passPath + "ic/fused_jump_target.mt");

        // ====================================
        // COMMENTED OUT - Test files were not created
        // ====================================
        /*
        addOutputVerificationTest("Await Lambda Basic",
                        passPath + "awaitLambdaBasic.mt");
        addOutputVerificationTest("Await Lambda Chain",
                        passPath + "awaitLambdaChain.mt");
        addOutputVerificationTest("Await Lambda Closure",
                        passPath + "awaitLambdaClosure.mt");
        addOutputVerificationTest("Await Lambda Generic",
                        passPath + "awaitLambdaGeneric.mt");
        addOutputVerificationTest("Await Lambda Parallel",
                        passPath + "awaitLambdaParallel.mt");
        addOutputVerificationTest("Await Lambda Nested",
                        passPath + "awaitLambdaNested.mt");
        addOutputVerificationTest("Await Lambda Composition",
                        passPath + "awaitLambdaComposition.mt");
        addOutputVerificationTest("Interface Generics Import Basic",
                        passPath + "interfaceGenericsImportBasic.mt");
        addOutputVerificationTest("Interface Generics Import Complex",
                        passPath + "interfaceGenericsImportComplex.mt");
        addOutputVerificationTest("Interface Generics Import Chain",
                        passPath + "interfaceGenericsImportChain.mt");
        addOutputVerificationTest("Interface Generics Import Diamond",
                        passPath + "interfaceGenericsImportDiamond.mt");
        addOutputVerificationTest("Interface Generics Import Wildcard",
                        passPath + "interfaceGenericsImportWildcard.mt");
        addOutputVerificationTest("Interface Generics Import Nested",
                        passPath + "interfaceGenericsImportNested.mt");
        addTestFromFile("Interface Generics Import Mismatch Error",
                        passPath + "interfaceGenericsImportMismatch.mt",
                        TestType::ERROR_EXPECTED);
        addOutputVerificationTest("Lambda Interface Generics Basic",
                        passPath + "lambdaInterfaceGenericsBasic.mt");
        addOutputVerificationTest("Lambda Interface Generics Functional",
                        passPath + "lambdaInterfaceGenericsFunctional.mt");
        addOutputVerificationTest("Lambda Interface Generics Chain",
                        passPath + "lambdaInterfaceGenericsChain.mt");
        addOutputVerificationTest("Lambda Interface Generics Composition",
                        passPath + "lambdaInterfaceGenericsComposition.mt");
        addOutputVerificationTest("Lambda Interface Generics Type Inference",
                        passPath + "lambdaInterfaceGenericsTypeInference.mt");
        addOutputVerificationTest("Lambda Interface Generics Bounded",
                        passPath + "lambdaInterfaceGenericsBounded.mt");
        addOutputVerificationTest("Await Interface Basic",
                        passPath + "awaitInterfaceBasic.mt");
        addOutputVerificationTest("Await Interface Generic",
                        passPath + "awaitInterfaceGeneric.mt");
        addOutputVerificationTest("Await Interface Polymorphism",
                        passPath + "awaitInterfacePolymorphism.mt");
        addOutputVerificationTest("Await Interface Chain",
                        passPath + "awaitInterfaceChain.mt");
        addOutputVerificationTest("Await Interface Parallel",
                        passPath + "awaitInterfaceParallel.mt");
        addOutputVerificationTest("All Features Basic",
                        passPath + "allFeaturesBasic.mt");
        addOutputVerificationTest("All Features Complex",
                        passPath + "allFeaturesComplex.mt");
        addOutputVerificationTest("All Features Event System",
                        passPath + "allFeaturesEventSystem.mt");
        addOutputVerificationTest("All Features Data Pipeline",
                        passPath + "allFeaturesDataPipeline.mt");
        addOutputVerificationTest("All Features State Machine",
                        passPath + "allFeaturesStateMachine.mt");
        addOutputVerificationTest("Integration Safe Cast Pattern",
                        passPath + "integrationSafeCastPattern_pass.mt");
        addOutputVerificationTest("Integration Cast Cleanup Failed",
                        passPath + "integrationCastCleanupFailed_pass.mt");
        addOutputVerificationTest("Integration Cast Retry",
                        passPath + "integrationCastRetry_pass.mt");
        addOutputVerificationTest("Integration Cast Finally",
                        passPath + "integrationCastFinally_pass.mt");
        addOutputVerificationTest("Integration Type Narrowing Control",
                        passPath + "integrationTypeNarrowingControl_pass.mt");
        addOutputVerificationTest("Integration Loop Type Inference",
                        passPath + "integrationLoopTypeInference_pass.mt");
        addOutputVerificationTest("Integration Switch Type Guard",
                        passPath + "integrationSwitchTypeGuard_pass.mt");
        addOutputVerificationTest("Integration Polymorphic Dispatch",
                        passPath + "integrationPolymorphicDispatch_pass.mt");
        addOutputVerificationTest("Integration Lambda Type Flow",
                        passPath + "integrationLambdaTypeFlow_pass.mt");
        addOutputVerificationTest("Integration Cast In Loop",
                        passPath + "integrationCastInLoop_pass.mt");
        addOutputVerificationTest("Integration Cast In Conditional",
                        passPath + "integrationCastInConditional_pass.mt");
        addOutputVerificationTest("Integration Cast In Recursion",
                        passPath + "integrationCastInRecursion_pass.mt");
        addOutputVerificationTest("Integration Cast In Async",
                        passPath + "integrationCastInAsync_pass.mt");
        addOutputVerificationTest("Integration All Generic Control",
                        passPath + "integrationAllGenericControl_pass.mt");
        addOutputVerificationTest("Integration All Async Casting",
                        passPath + "integrationAllAsyncCasting_pass.mt");
        addOutputVerificationTest("Integration All Type Safe Error",
                        passPath + "integrationAllTypeSafeError_pass.mt");
        addOutputVerificationTest("Integration All Feature Showcase",
                        passPath + "integrationAllFeatureShowcase_pass.mt");
        */
        ;
    }
}
