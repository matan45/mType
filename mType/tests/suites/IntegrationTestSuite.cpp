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
        addOutputVerificationTest("Control Flow With Class Members And Namespaces",
                        passPath + "controlFlowWithClassMembersAndNamespaces.mt");
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
        addOutputVerificationTest("Final Variables Across Namespaces And Classes",
                        passPath + "finalVariablesAcrossNamespacesAndClasses.mt");
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
        addOutputVerificationTest("Test Type Substitution Edge Cases",
                        passPath + "test_type_substitution_edge_cases.mt");
        addOutputVerificationTest("Type System Edge Cases",
                        passPath + "typeSystemEdgeCases.mt");

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
        */;
    }
}

