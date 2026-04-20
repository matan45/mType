#include "EscapeAnalysisTestSuite.hpp"

namespace tests::testSuite
{
    using namespace testFramework;

    void EscapeAnalysisTestSuite::setupTests()
    {
        // Promotable: allocation bound to a local, only used for field read.
        addOutputVerificationTest("Tight Local",
                        passPath + "tight_local.mt");

        // Promotable: union-find aliasing keeps both locals stack-allocated.
        addOutputVerificationTest("Alias Chain",
                        passPath + "alias_chain.mt");

        // Promotable: hot loop allocation (same pattern as object_alloc benchmark).
        addOutputVerificationTest("Loop Local",
                        passPath + "loop_local.mt");

        // Promotable: method call on the local (receiver is SAFE).
        addOutputVerificationTest("Method Call On Self",
                        passPath + "method_call_on_self.mt");

        // Promotable: allocation inside a conditional branch.
        addOutputVerificationTest("Conditional Local",
                        passPath + "conditional_local.mt");

        // Escapes via return — must stay heap. Output must still be correct.
        addOutputVerificationTest("Returned Escapes",
                        passPath + "returned.mt");

        // Escapes via field store — must stay heap.
        addOutputVerificationTest("Field Store Escapes",
                        passPath + "field_store.mt");

        // Escapes via lambda capture — must stay heap.
        addOutputVerificationTest("Lambda Capture Escapes",
                        passPath + "lambda_capture.mt");

        // Escapes via throw — must stay heap.
        addOutputVerificationTest("Thrown Escapes",
                        passPath + "thrown.mt");

        // Escapes via being passed as a function call argument — must stay heap.
        addOutputVerificationTest("Passed As Arg Escapes",
                        passPath + "passed_as_arg.mt");

        // Aliased then returned — union-find must escalate both locals to escaped.
        addOutputVerificationTest("Aliased Then Returned",
                        passPath + "aliased_then_returned.mt");
    }
}
