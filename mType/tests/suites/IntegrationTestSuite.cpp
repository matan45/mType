#include "IntegrationTestSuite.hpp"

namespace tests::testSuite
{
    using namespace testFramework;
    
    void IntegrationTestSuite::setupTests()
    {
        // Complex Feature Integration tests
        addTestFromFile("Namespaces with Classes and Imports",
                        passPath + "namespacesWithClassesAndImports.mt");
        addTestFromFile("Classes with Final Variables and Loops",
                        passPath + "classesWithFinalVariablesAndLoops.mt");
        addTestFromFile("Nested Namespaces with Static Methods",
                        passPath + "nestedNamespacesWithStaticMethods.mt");
        addTestFromFile("Control Flow with Class Members and Namespaces",
                        passPath + "controlFlowWithClassMembersAndNamespaces.mt");
        addTestFromFile("Imports with Namespaces and Classes",
                        passPath + "importsWithNamespacesAndClasses.mt");
        addTestFromFile("Complex Inheritance with Namespaces",
                        passPath + "complexInheritanceWithNamespaces.mt");
        addTestFromFile("Final Variables Across Namespaces and Classes",
                        passPath + "finalVariablesAcrossNamespacesAndClasses.mt");
        addTestFromFile("Loops with Class Creation and Namespaces",
                        passPath + "loopsWithClassCreationAndNamespaces.mt");
        addTestFromFile("Recursive Namespace Class Interactions",
                        passPath + "recursiveNamespaceClassInteractions.mt");
        addTestFromFile("Concurrent Namespace and Class Modifications",
                        passPath + "concurrentNamespaceAndClassModifications.mt");
        
        // Edge Cases and Potential Bugs tests
        addTestFromFile("Circular Namespace References",
                        passPath + "circularNamespaceReferences.mt");
        addTestFromFile("Namespace Class Name Collisions",
                        passPath + "namespaceClassNameCollisions.mt");
        addTestFromFile("Deep Nesting Limits",
                        passPath + "deepNestingLimits.mt");
        addTestFromFile("Memory Leaks with Complex Objects",
                        passPath + "memoryLeaksWithComplexObjects.mt");
        addTestFromFile("Large Data Structures",
                        passPath + "largeDataStructures.mt");
        
        // Object Destruction Order and Cleanup Tests
        addTestFromFile("Object Destruction Order Basic",
                        passPath + "objectDestructionOrderBasic.mt");
        addTestFromFile("Complex Object Cleanup Scenarios",
                        passPath + "complexObjectCleanupScenarios.mt");
        addTestFromFile("Object Lifecycle Edge Cases",
                        passPath + "objectLifecycleEdgeCases.mt");
        addTestFromFile("Memory Leak Prevention",
                        passPath + "memoryLeakPrevention.mt");
        addTestFromFile("Resource Cleanup Patterns",
                        passPath + "resourceCleanupPatterns.mt");
        addTestFromFile("Exception Handling Across Features",
                        errorPath + "exceptionHandlingAcrossFeatures.mt",
                        TestType::ERROR_EXPECTED);
        addTestFromFile("Type System Edge Cases",
                        passPath + "typeSystemEdgeCases.mt");
        
        // Performance and Stress tests
        addTestFromFile("Performance with Large Namespaces",
                        passPath + "performanceWithLargeNamespaces.mt");
        addTestFromFile("Stress Test with Many Classes",
                        passPath + "stressTestWithManyClasses.mt");
        addTestFromFile("Complex Control Flow Performance",
                        passPath + "complexControlFlowPerformance.mt");
        addTestFromFile("Memory Usage with Deep Nesting",
                        passPath + "memoryUsageWithDeepNesting.mt");
        
        // Error Handling and Recovery tests
        addTestFromFile("Error Recovery with Partial Failures",
                        passPath + "errorRecoveryWithPartialFailures.mt");
        addTestFromFile("Invalid Combinations Handling",
                        passPath + "invalidCombinationsHandling.mt");
        addTestFromFile("Corrupted State Recovery",
                        passPath + "corruptedStateRecovery.mt");
    }
}

