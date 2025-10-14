#include "OptimizerTestSuite.hpp"

namespace tests::testSuite
{
    using namespace testFramework;

    void OptimizerTestSuite::setupTests()
    {
        // Dead Code Elimination tests
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
    }
}
