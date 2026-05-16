// MYT-322: free-function direct JIT-to-JIT dispatch, re-landed with two
// gates that the original MYT-321 attempt lacked — pre-cached IC fields
// (frameName / programIndex / JitFn ptr) so the warm hit avoids the
// hashmap probe + loadedPrograms scan, and a callee-size threshold
// (MIN_DIRECT_CALL_INSTRUCTION_COUNT) so tiny static callees stay on the
// cheaper mini-interpret path that originally regressed +56% under
// generic_dispatch_hot.
//
// This test covers three behaviours through the user-visible result
// channel (correctness only — no perf assertions, those live in the
// benchmark suite):
//
//   1. Large callee (instructionCount comfortably above the threshold)
//      hot-called in a tight loop. Once the callee crosses the function-
//      level JIT hot threshold, the warm-path branch in
//      jit_call_function_ic routes through jit_call_function_direct.
//      Result must match the interpreter-side reference value computed
//      ahead of warmup.
//   2. Tiny callee (~3 bytecode instructions) hot-called in a tight
//      loop. cachedFuncMetadata->instructionCount stays under the
//      threshold so the warm hit falls through to
//      callFunctionFromJitDirect's mini-interpret. Result must still
//      match the reference.
//   3. A large callee throws on a specific input value, caught at the
//      caller. Exercises the helper's pendingException propagation
//      across the direct-dispatch boundary (mirrors the channel
//      jit_call_method_direct uses).

import * from "../../lib/exceptions/Exception.mt";

// Tiny callee: ~3 instructions (LOAD_LOCAL, PUSH_CONST_INT, ADD_INT,
// RETURN). Stays below MIN_DIRECT_CALL_INSTRUCTION_COUNT, so warm
// hits at its call site go through callFunctionFromJitDirect.
function tinyCallee(int n): int {
    return n + 1;
}

// Large callee: instructionCount well above 15 (multiple ADD/MUL/SUB
// per assignment, plus LOAD_LOCAL/STORE_LOCAL pairs). Crosses the
// threshold so warm hits at its call site go through
// jit_call_function_direct.
function bigCallee(int n): int {
    int a = n + 1;
    int b = n * 2;
    int c = n - 3;
    int d = a + b;
    int e = b * c;
    int f = d - e;
    int g = a * b;
    int h = c + d;
    int i = e - f;
    int j = g + h;
    return a + b + c + d + e + f + g + h + i + j;
}

// Large callee that throws on a sentinel input. Used to exercise
// pendingException propagation through the direct-dispatch boundary.
function bigCalleeThrowing(int n): int {
    int a = n + 1;
    int b = n * 2;
    int c = n - 3;
    int d = a + b;
    int e = b * c;
    if (n == 99) {
        throw new Exception("BigCalleeBoom");
    }
    int f = d - e;
    int g = a + b + c;
    return d + e + f + g;
}

function main(): void {
    // Reference values: computed once before warmup, off the hot path
    // (the JIT hasn't compiled either function yet). After warmup the
    // hot path either takes direct dispatch (bigCallee) or
    // mini-interpret (tinyCallee), and the result must still match.
    int tinyRef = tinyCallee(7);
    int bigRef = bigCallee(5);

    // Warmup loop: comfortably above the function-level JIT hot
    // threshold (default 100). After the loop, both callees are JIT-
    // compiled, both IC slots are populated with cachedJitFnPtr +
    // cachedFrameName + cachedProgramIndex.
    int tinySum = 0;
    int bigSum = 0;
    for (int i = 0; i < 1000; i = i + 1) {
        tinySum = tinySum + tinyCallee(i);
        bigSum = bigSum + bigCallee(i);
    }

    // Direct-dispatch (bigCallee) and mini-interpret (tinyCallee) hot
    // hits must produce the same value as the cold reference call.
    print("tiny matches ref: " + (tinyCallee(7) == tinyRef));
    print("big matches ref: " + (bigCallee(5) == bigRef));
    print("tiny sum: " + tinySum);
    print("big sum: " + bigSum);

    // Exception propagation through jit_call_function_direct.
    // Warm bigCalleeThrowing first so the warm-path direct-dispatch
    // branch is the one that handles the throw on the sentinel.
    int throwingSum = 0;
    for (int i = 0; i < 1000; i = i + 1) {
        if (i == 99) {
            // Skip the throwing input during warmup so the JIT compiles
            // a non-throwing shape first.
            throwingSum = throwingSum + 0;
        } else {
            throwingSum = throwingSum + bigCalleeThrowing(i);
        }
    }

    bool caught = false;
    string msg = "";
    try {
        int x = bigCalleeThrowing(99);
        print("FAIL: expected throw, got " + x);
    } catch (Exception e) {
        caught = true;
        msg = e.getMessage();
    }
    print("throw caught: " + caught);
    print("throw msg: " + msg);
    print("done");
}

main();
