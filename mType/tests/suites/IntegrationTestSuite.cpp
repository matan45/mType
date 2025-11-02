#include "IntegrationTestSuite.hpp"

namespace tests::testSuite
{
    using namespace testFramework;
    
    void IntegrationTestSuite::setupTests()
    {

        addOutputVerificationTest("Conversion basic",
                        passPath + "test_type_conversion_basic.mt");
        addOutputVerificationTest("Generics simple",
                        passPath + "test_simple_generics.mt");
        addOutputVerificationTest("Arrays simple",
                        passPath + "test_array_types.mt");
        addOutputVerificationTest("Error Handling comprehensive",
                        passPath + "test_error_handling.mt");
        // Complex Feature Integration tests
        addOutputVerificationTest("Namespaces with Classes and Imports",
                        passPath + "namespacesWithClassesAndImports.mt");
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
        // Await + Lambda Integration Tests
        // ====================================
        addOutputVerificationTest("Await Lambda Basic",
                        passPath + "awaitLambdaBasic.mt");
        addOutputVerificationTest("Await Lambda Chain",
                        passPath + "awaitLambdaChain.mt");
        addOutputVerificationTest("Await Lambda Closure",
                        passPath + "awaitLambdaClosure.mt");
        addOutputVerificationTest("Await Lambda Generic",
                        passPath + "awaitLambdaGeneric.mt");
        addOutputVerificationTest("Await Lambda Exception Handling",
                        passPath + "awaitLambdaExceptionHandling.mt");
        addOutputVerificationTest("Await Lambda Parallel",
                        passPath + "awaitLambdaParallel.mt");
        addOutputVerificationTest("Await Lambda Nested",
                        passPath + "awaitLambdaNested.mt");
        addOutputVerificationTest("Await Lambda Composition",
                        passPath + "awaitLambdaComposition.mt");

        // ====================================
        // Interface + Generics + Import Integration Tests
        // ====================================
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

        // Interface + Generics + Import error test
        addTestFromFile("Interface Generics Import Mismatch Error",
                        passPath + "interfaceGenericsImportMismatch.mt",
                        TestType::ERROR_EXPECTED);

        // ====================================
        // Lambda + Interface + Generics Integration Tests
        // ====================================
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

        // ====================================
        // Await + Interface Integration Tests
        // ====================================
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

        // ====================================
        // All Features Integration Tests
        // ====================================
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

        // ====================================
        // Arrays + Generics Integration Tests
        // ====================================
        addOutputVerificationTest("Array Generic Constraints",
                        passPath + "arrayGenericConstraints.mt");
        addOutputVerificationTest("Array Generic Complex",
                        passPath + "arrayGenericComplex.mt");
        addOutputVerificationTest("Array Generic Methods",
                        passPath + "arrayGenericMethods.mt");
        addOutputVerificationTest("Array Generic Collection",
                        passPath + "arrayGenericCollection.mt");
        addOutputVerificationTest("Array Generic SIMD",
                        passPath + "arrayGenericSIMD.mt");
        addOutputVerificationTest("Array Generic Multi Dimensional",
                        passPath + "arrayGenericMultiDim.mt");

        // ====================================
        // Class + Generics Integration Tests
        // ====================================
        addOutputVerificationTest("Class Generic Inheritance Chain",
                        passPath + "classGenericInheritanceChain.mt");
        addOutputVerificationTest("Class Generic Multi Interface",
                        passPath + "classGenericMultiInterface.mt");
        addOutputVerificationTest("Class Generic Overloading",
                        passPath + "classGenericOverloading.mt");
        addOutputVerificationTest("Class Generic Builder",
                        passPath + "classGenericBuilder.mt");
        addOutputVerificationTest("Class Generic Factory",
                        passPath + "classGenericFactory.mt");
        addOutputVerificationTest("Class Generic Singleton",
                        passPath + "classGenericSingleton.mt");

        // ====================================
        // Arrays + Class Integration Tests
        // ====================================
        addOutputVerificationTest("Array Class Polymorphic",
                        passPath + "arrayClassPolymorphic.mt");
        addOutputVerificationTest("Array Class Inheritance",
                        passPath + "arrayClassInheritance.mt");
        addOutputVerificationTest("Array Class Interfaces",
                        passPath + "arrayClassInterfaces.mt");
        addOutputVerificationTest("Array Class Return Values",
                        passPath + "arrayClassReturnValues.mt");

        // ====================================
        // All Three Features Combined Tests
        // ====================================
        addOutputVerificationTest("All Generic Array Fields",
                        passPath + "allGenericArrayFields.mt");
        addOutputVerificationTest("All Generic Array Methods",
                        passPath + "allGenericArrayMethods.mt");
        addOutputVerificationTest("All Generic Inheritance Arrays",
                        passPath + "allGenericInheritanceArrays.mt");
        addOutputVerificationTest("All Complete Integration",
                        passPath + "allCompleteIntegration.mt");
    }
}

