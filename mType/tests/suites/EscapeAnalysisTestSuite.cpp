#include "EscapeAnalysisTestSuite.hpp"

#include <stdexcept>
#include <string>

#include "../../services/ScriptAPI.hpp"
#include "../../value/ObjectInstancePool.hpp"
#include "../../value/ValueShim.hpp"

namespace tests::testSuite
{
    using namespace testFramework;
    using services::ScriptAPI;

    namespace
    {
        // NATIVE_CALLBACK runner catches std::exception and fails the test
        // with the message — mirrors the helper in ScriptApiNativeTestSuite.
        void require(bool cond, const std::string& msg)
        {
            if (!cond)
            {
                throw std::runtime_error(msg);
            }
        }
    }

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

        // MYT-208 nested-helper allocation pattern — primary perf target.
        // Both Points bounded by distanceSq's frame; analyzer promotes to
        // NEW_STACK and pool slots recycle on every iteration.
        addOutputVerificationTest("Nested Helper Alloc",
                        passPath + "nested_helper_alloc.mt");

        // MYT-208 super(...) on stack-promoted derived ctor — covers
        // SUPER_CONSTRUCTOR's tag-aware frame setup (thisInstanceRaw
        // propagation to parent ctor frame).
        addOutputVerificationTest("Super Ctor Stack",
                        passPath + "super_ctor_stack.mt");

        // MYT-208 monomorphic method call on a stack-promoted local —
        // covers IC dispatch with STACK_OBJECT receiver.
        addOutputVerificationTest("Method Call On Promoted",
                        passPath + "method_call_on_promoted.mt");

        // MYT-352 canary: when the JIT inliner accepts a NEW_STACK-containing
        // callee, the inlined NEW_STACK must be bounded by per-block
        // STACK_SCOPE_ENTER / LEAVE wrapping that the compiler emits around
        // *every* block containing a promoted NEW (function-body blocks
        // included) plus return-time scope drainage. Without those landings
        // the inlined NEW_STACKs accumulate in the caller's stackObjects[],
        // saturate the 32-slot cap after ~16 iterations, and fall back to
        // the heap path — pool hit rate plummets and poolMisses grow with
        // iteration count. Run a non-trivial workload through the inliner
        // and assert pool stats stay healthy.
        addCallbackTest("MYT-352 Inlined NEW_STACK Pool Reuse",
            passPath + "inlined_new_stack_pool_reuse.mt",
            [](ScriptAPI& api) {
                constexpr int64_t kIters = 5000;
                auto& pool = value::ObjectInstancePool::getInstance();
                value::ObjectPoolStats before = pool.getGlobalStats();
                api.callFunction("runWorkload", { value::Value(int64_t{kIters}) });
                value::ObjectPoolStats after = pool.getGlobalStats();

                const size_t deltaAllocs = after.totalAllocations - before.totalAllocations;
                const size_t deltaHits   = after.poolHits         - before.poolHits;
                const size_t deltaMisses = after.poolMisses       - before.poolMisses;

                // Two Point allocations per iteration; allow a small slack
                // for any incidental allocations the runtime threads through
                // (none expected, but be defensive).
                require(deltaAllocs >= static_cast<size_t>(2 * kIters) - 8,
                    "expected ~2 ObjectInstancePool allocations per workload "
                    "iteration; got " + std::to_string(deltaAllocs));

                const double hitRate = deltaAllocs > 0
                    ? static_cast<double>(deltaHits) /
                      static_cast<double>(deltaAllocs)
                    : 0.0;
                require(hitRate > 0.9,
                    "expected pool hit rate > 0.9 after warmup (pool slots "
                    "should recycle per inlined-call); got hitRate=" +
                    std::to_string(hitRate) + " hits=" +
                    std::to_string(deltaHits) + " allocs=" +
                    std::to_string(deltaAllocs));

                // poolMisses is bounded by per-class bucket warm-up only —
                // a tiny constant, independent of kIters. Climbing linearly
                // with kIters is the regression signature (cap saturated →
                // heap fallback → fresh ::operator new every iteration).
                require(deltaMisses < 32,
                    "poolMisses should stay tiny after warmup (regression "
                    "signature: cap saturation forces heap fallback); got " +
                    std::to_string(deltaMisses));
            });
    }
}
