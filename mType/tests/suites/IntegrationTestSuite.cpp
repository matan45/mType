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
    }
}

