#include "GCTestSuite.hpp"

namespace tests::testSuite
{
    using namespace testFramework;

    void GCTestSuite::setupTests()
    {
        // === BASIC CYCLE TESTS ===
        // Tests for fundamental circular reference patterns

        addOutputVerificationTest("Simple Cycle (A -> B -> A)",
                                  passPath + "simpleCycle.mt");

        addOutputVerificationTest("Self-Reference (A -> A)",
                                  passPath + "selfReference.mt");

        addOutputVerificationTest("Deep Cycle (A -> B -> C -> D -> A)",
                                  passPath + "deepCycle.mt");

        // === LAMBDA CAPTURE CYCLES ===
        // Tests for circular references involving lambda closures

        addOutputVerificationTest("Lambda Capture Cycle",
                                  passPath + "lambdaCycle.mt");

        // === COLLECTION CYCLES ===
        // Tests for circular references in collection structures

        addOutputVerificationTest("LinkedList Cycle",
                                  passPath + "linkedListCycle.mt");

        // === MIXED REACHABILITY ===
        // Tests with some objects reachable and some in cycles

        addOutputVerificationTest("Mixed Reachability",
                                  passPath + "mixedReachability.mt");

        // === STRESS TESTS ===
        // Tests with many objects and cycles

        addOutputVerificationTest("Stress Test (5000 nodes)",
                                  passPath + "stressTestAbort.mt");

        addOutputVerificationTest("Stress Test Large (20000 nodes)",
                                  passPath + "stressTestAbortLarge.mt");

        addOutputVerificationTest("Force Abort (50000 nodes)",
                                  passPath + "forceAbort.mt");

        // === SURVIVAL UNDER ALLOCATION PRESSURE ===
        // The existing tests prove garbage IS collected; these prove live
        // data is NOT — strings and collection-internal objects must survive
        // the GC cycles triggered by churning 40k cyclic garbage objects.
        addOutputVerificationTest("String Pool Survives GC Pressure",
                                  passPath + "gcStringPoolSurvives.mt");

        addOutputVerificationTest("Objects In Collections Survive GC Pressure",
                                  passPath + "gcObjectsInCollectionsSurvive.mt");
    }
}
