#include "OptimizerTestSuite.hpp"

namespace tests::testSuite
{
    using namespace testFramework;

    void OptimizerTestSuite::setupTests()
    {
        // Dead Code Elimination - Basic tests
        addOutputVerificationTest("Dead Code After Return",
                        passPath + "deadCodeAfterReturn.mt");
        addOutputVerificationTest("Dead Code After Break",
                        passPath + "deadCodeAfterBreak.mt");
        addOutputVerificationTest("Dead Code After Continue",
                        passPath + "deadCodeAfterContinue.mt");
        addOutputVerificationTest("Dead Code After Throw",
                        passPath + "deadCodeAfterThrow.mt");
        addOutputVerificationTest("Nested Dead Code",
                        passPath + "nestedDeadCode.mt");
        addOutputVerificationTest("Dead Code in If Branches",
                        passPath + "deadCodeInIfBranches.mt");
        addOutputVerificationTest("No Dead Code (Baseline)",
                        passPath + "noDeadCode.mt");

        // Dead Code Elimination - Control flow tests
        addOutputVerificationTest("Dead Code in If-Else",
                        passPath + "deadCodeIfElse.mt");
        addOutputVerificationTest("Dead Code in Switch",
                        passPath + "deadCodeSwitch.mt");
        addOutputVerificationTest("Dead Code in Loops",
                        passPath + "deadCodeLoops.mt");

        // Dead Code Elimination - Advanced features
        addOutputVerificationTest("Dead Code in Catch Blocks",
                        passPath + "deadCodeInCatch.mt");
        addOutputVerificationTest("Dead Code in Lambda",
                        passPath + "deadCodeInLambda.mt");
        addOutputVerificationTest("Dead Code in Finally",
                        passPath + "deadCodeInFinally.mt");
        addOutputVerificationTest("Dead Code in Async Functions",
                        passPath + "deadCodeInAsync.mt");

        // Dead Code Elimination - Inheritance and interfaces
        addOutputVerificationTest("Dead Code in Inheritance (Simple)",
                        passPath + "deadCodeInheritanceSimple.mt");
        addOutputVerificationTest("Dead Code in Inheritance and Interfaces",
                        passPath + "deadCodeInheritanceInterfaces.mt");

        // Unused Declaration Elimination tests
        addOutputVerificationTest("Unused Declarations",
                        passPath + "unusedDeclarations.mt");
        addOutputVerificationTest("Unused Methods",
                        passPath + "unusedMethods.mt");
        addOutputVerificationTest("Unused Class Dependencies",
                        passPath + "unusedClassDependencies.mt");
        addOutputVerificationTest("Test Import Optimization",
                        passPath + "testImportOptimization.mt");

        // Other Optimization tests
        addOutputVerificationTest("Array Optimization",
                        passPath + "arrayOptimization.mt");
        addOutputVerificationTest("Switch Optimization",
                        passPath + "switchOptimization.mt");
    }
}
