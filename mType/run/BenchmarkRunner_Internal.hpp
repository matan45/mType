#pragma once
#include <array>
#include <cstdint>
#include <cstddef>
#include <string>
#include <vector>
#include "../vm/runtime/context/ExecutionContext.hpp"
#include "../vm/jit/OSRManager.hpp"

// Private support header for BenchmarkRunner.cpp + BenchmarkRunner_Report.cpp.
// Hosts the shared result types that flow from measurement into formatting;
// not included anywhere outside mType/run/.

namespace runMain
{
namespace detail
{
    constexpr const char* BENCHMARKS_REL = "mType/tests/testFiles/benchmarks";

    inline constexpr std::array<const char*, 61> CANONICAL_SCRIPTS = {
        "arithmetic_tight_loop.mt",
        "method_dispatch.mt",
        "object_alloc.mt",
        // MYT-208: short-scope allocations inside a helper called in a hot
        // loop — the workload class where escape-analysis Phase 2 wins.
        "object_alloc_nested.mt",
        // Repeated short-lived cyclic object graphs. Exercises normal GC
        // safepoints and cycle detection under allocation churn.
        "gc_cycle_churn.mt",
        // MYT-191: direct-field-write hot loop. SET_FIELD lives inside the
        // OSR-compiled outer loop (unlike object_alloc.mt, whose SETs are
        // in Point.constructor and run interpreted), so this is the only
        // canonical script that exercises tryEmitInlinedFieldSet.
        "field_write_hot.mt",
        // MYT-194: mirror of field_write_hot for the GET path. Primary
        // acceptance benchmark for the GET_FIELD_CACHED interpreter fast
        // path (and tryEmitInlinedFieldGet on the JIT side).
        "field_read_hot.mt",
        "polymorphic_field_hot.mt",
        "string_ops.mt",
        "recursive.mt",
        "bitwise_tight_loop.mt",
        "short_circuit_chain.mt",
        "primitive_method_dispatch.mt",
        "array_multi_alloc.mt",
        "array_multi_get.mt",
        "for_each_loop.mt",
        // MYT-163 F-a: primary acceptance target — single-class MONO call
        // site eligible for speculative inlining.
        "inline_monomorphic.mt",
        // MYT-164 F-b: MONO callee with an internal if-guard (mirrors the
        // iterator hasNext / null-check pattern). Un-inlineable under F-a
        // due to HAS_INTERNAL_JUMPS; inlined under F-b via per-frame label
        // remapping.
        "inline_branching.mt",
        // MYT-165 F-c: 4-subclass polymorphic hot loop (fills
        // IC_MAX_POLYMORPHIC_ENTRIES = 4). Exercises the chained shape-guard
        // emission against the maximum IC-width case.
        "inline_polymorphic.mt",
        // MYT-173 follow-up: mixed-inlineability POLY-4 site. Three small
        // shapes inline, one oversized shape routes through the per-shape
        // helper branch in emitInlinedMethodCallPoly. Pre-change every
        // dispatch ran jit_call_method_ic because the all-or-nothing
        // eligibility gate rejected the whole site; primary regression
        // benchmark for the per-entry POLY eligibility relaxation.
        "inline_polymorphic_mixed.mt",
        // MYT-173 follow-up: 6 subclasses overflow IC_MAX_POLYMORPHIC_ENTRIES
        // (= 4) and drive the IC to MEGAMORPHIC. Baseline for the MEGA cliff —
        // every dispatch hits jit_call_method_ic with no inlined fast path.
        "megamorphic_dispatch.mt",
        // MYT-167 F-e: value-class MONO hot loop. Pre-F-e the slow path was
        // jit_call_method's temp-ObjectInstance materialisation per call;
        // post-F-e the IC populates with receiverIsValueObject=true and the
        // body inlines identically to inline_monomorphic.mt.
        "inline_value_object_hot.mt",
        // MYT-210: plain-CALL inliner acceptance benchmark. 4-arg leaf
        // distanceSq in a tight 2 M-iter loop — measures the per-call
        // overhead removed by inlining versus the jit_call_function helper.
        "function_call_hot.mt",
        // MYT-314: hot non-looping leaf-methods tier up via the function-
        // entry path (PROFILE_ENTER -> JitCompiler::compile). Three small
        // leaves with no internal loop; OSR cannot see them and the plain-
        // function inliner is currently disabled, so the only path to
        // native code is MYT-314's tier-up. Companion to function_call_hot
        // (which measures the inliner once it returns).
        "function_entry_tier_hot.mt",
        // MYT-322: free-function direct JIT-to-JIT dispatch canary.
        // Callee `mediumCompute` is ~30 bytecode instructions — above
        // MIN_DIRECT_CALL_INSTRUCTION_COUNT — so once function-entry
        // tiering JIT-compiles it the warm path in jit_call_function_ic
        // refreshes its cachedJitFnPtr and routes through
        // jit_call_function_direct. The four existing function-call
        // benchmarks all have callees ≤ 4 instructions so they exercise
        // only the mini-interpret fallback; this benchmark is the
        // dedicated coverage for the direct-dispatch path.
        "function_call_medium_hot.mt",
        // Async/await suspend-resume cost in a tight await loop.
        "async_await_tight_loop.mt",
        // Sequential await chain — 4 awaits/iter, distinct continuation shapes.
        "async_await_chain.mt",
        // Lambda invocation through a single-method interface in a hot loop.
        "lambda_call_hot.mt",
        // Closure-capture read overhead on the lambda hot path.
        "lambda_closure_hot.mt",
        // MYT-228: hot loop dispatching `isClassOf T` through a method-level
        // generic helper. Exercises BIND_TYPE_ARGS staging,
        // CallFrame::typeArgBindings consume, and the
        // jit_instanceof_typeparam JIT helper. Acceptance: per-call overhead
        // stays close to method_dispatch.mt — the per-frame map lookup is
        // not on the non-generic hot path, so non-generic benchmarks must
        // not regress.
        "generic_dispatch_hot.mt",
        // Runtime feature-surface coverage beyond the core VM hot paths above.
        "try_catch_finally_hot.mt",
        "switch_dispatch_hot.mt",
        "overload_dispatch_hot.mt",
        "abstract_dispatch_hot.mt",
        "cast_hot.mt",
        "collections_hash_hot.mt",
        "collections_hash_user_class_hot.mt",
        "collections_hashset_hot.mt",
        "stream_pipeline_hot.mt",
        "reflection_lookup_hot.mt",
        "pattern_match_hot.mt",
        "string_interpolation_hot.mt",
        "boxed_primitive_dispatch_hot.mt",
        // Isolation benchmarks for the boxed-primitive INVOKE_BOOL_* /
        // INVOKE_STRING_* fast paths and the StringPool intern hit rate.
        // Companions to boxed_primitive_dispatch_hot.mt — staying pure-Bool
        // / pure-String makes regressions in either path attributable.
        "boxed_bool_dispatch_hot.mt",
        "boxed_string_dispatch_hot.mt",
        // Box/unbox pressure micro: chases an int through Box<Int>(new Int(i))
        // every iteration so allocation + generic-field T=Int load + value-class
        // unwrap form a clean signal for the JitCompiler_Boxing.cpp emit surface.
        // Companion to boxed_*_dispatch_hot — those measure method dispatch on
        // boxed receivers; this one isolates the boxing-pressure cost.
        "box_unbox_hot.mt",
        // Isolation benchmark for non-generic CALL_STATIC dispatch.
        // Companion to generic_dispatch_hot.mt; strips BIND_TYPE_ARGS and
        // INSTANCEOF_TYPEPARAM so the measured cost is purely the
        // jit_call_function_ic + interpreter-side cached-static-call path.
        // Should land near method_dispatch.mt (~125ms) once warmed.
        "static_call_hot.mt",
        // MYT-254: linked-list traversal under nested loops. Inner walks
        // all M elements via get(k), which chases Node.next pointers each
        // call — O(M^2) per outer iter, exercising field-chain inlining
        // at depth 2+ on a linked container (vs the ArrayList-backed
        // stream_pipeline_hot).
        "linked_list_nested_hot.mt",
        "method_chain_hot.mt",
        "string_build_call_hot.mt",
        // Phase 1: additional coverage for runtime-perf work driven by
        // docs/bench-baseline.md. Each measures a hot shape that the previous
        // 49-script set under-covered. See IntegrationTestSuite.cpp for the
        // matching .expected JIT-correctness pairs.
        "deep_inheritance_hot.mt",
        "interface_dispatch_collections_hot.mt",
        "cast_miss_hot.mt",
        "exception_throw_hot.mt",
        "ic_transition_hot.mt",
        "switch_on_string_hot.mt",
        "substring_hot.mt",
        "gc_churn_intense_hot.mt",
        "array_literal_alloc_hot.mt",
        // MYT-346: value_class_mut_hot currently fails under --jit (OSR
        // diverges from interpreter on value-class write methods past the
        // OSR threshold). Retained in the canonical list so --benchmark
        // reports wall-time on both paths; the integration suite uses it as
        // a regression canary.
        "value_class_mut_hot.mt",
        "int_only_arith_hot.mt",
    };

    struct JitFailedLoop
    {
        std::size_t jumpBackOffset = 0;
        vm::jit::OSRBailoutReason reason = vm::jit::OSRBailoutReason::NONE;
        std::uint8_t offendingOpcode = 0;
    };

    struct JitHotFunc
    {
        std::string name;
        std::uint32_t calls = 0;
        bool compiled = false;
    };

    struct JitSample
    {
        bool captured = false;
        std::size_t compileCount = 0;
        std::size_t bailoutCount = 0;
        std::size_t cachedFunctions = 0;
        std::size_t loopsProfiled = 0;
        std::size_t osrCompiled = 0;
        std::size_t osrFailed = 0;
        std::uint64_t inlineFieldICHits = 0;
        std::uint64_t inlineFieldICMisses = 0;
        std::array<std::uint64_t, 256> functionBailoutOpcodes = {};
        std::array<std::uint64_t, 256> osrBailoutOpcodes = {};
        std::vector<JitFailedLoop> failedLoops;
        std::vector<JitHotFunc> hotFunctions;
    };

    struct IterationSample
    {
        double wallMs = 0.0;
        vm::runtime::ExecutionStats stats{};
        std::size_t stringPoolRequests = 0;
        std::size_t stringPoolHits = 0;
        std::size_t arrayPoolAllocs = 0;
        std::size_t arrayPoolHits = 0;
        JitSample jit{};
    };

    struct ScriptResult
    {
        std::string path;
        std::string name;
        bool ok = true;
        std::string errorMessage;
        std::vector<IterationSample> measured;
    };

    struct Aggregate
    {
        double minMs = 0.0;
        double medianMs = 0.0;
        double meanMs = 0.0;
        double stddevMs = 0.0;
    };

    // Defined in BenchmarkRunner_Report.cpp.
    void printTextResult(const ScriptResult& r, bool jitEnabled);
    void printTextSummary(const std::vector<ScriptResult>& results, bool jitEnabled);
    void printJsonResults(const std::vector<ScriptResult>& results, bool jitEnabled);
}
}
