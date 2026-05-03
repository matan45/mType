// Drift detector for the hashCode()/equals() protocol fast path
// (commit d9c5d3c6: MethodProtocolFastKind in InlineCacheExecutor / JIT).
//
// The fast path bypasses the wrapper user-method bodies in
// lib/primitives/{Int,Float,Bool,String}.mt and computes the result
// directly in C++ (computePrimitiveProtocolHash / *Equals). The two
// implementations are coupled only by convention — nothing enforces
// that a future edit to a wrapper's hashCode/equals body stays
// consistent with the C++ helper.
//
// This test:
//   - For each wrapper class, calls wrapper.hashCode() many times
//     (first call: cold IC -> user method; subsequent calls: warm
//     MONO -> C++ fast path) and asserts the result matches the
//     ground-truth global hashCode(primitive) — which is what each
//     wrapper's body delegates to today.
//   - Calls wrapper.equals(other) on equal and unequal pairs and
//     asserts the result matches a direct primitive == comparison.
//
// If a future change makes a wrapper's hashCode/equals body diverge
// from "field 0 + std::hash / ==", update both this test and
// computePrimitiveProtocolHash/Equals together (or land the
// @PrimitiveProtocol annotation that makes the coupling explicit).

import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/Float.mt";
import * from "../../lib/primitives/Bool.mt";
import * from "../../lib/primitives/String.mt";

function checkIntHash(int v, int warmIters): bool {
    Int w = new Int(v);
    int h = w.hashCode();
    int i = 0;
    while (i < warmIters) {
        h = w.hashCode();
        i = i + 1;
    }
    return h == hashCode(v);
}

function checkFloatHash(float v, int warmIters): bool {
    Float w = new Float(v);
    int h = w.hashCode();
    int i = 0;
    while (i < warmIters) {
        h = w.hashCode();
        i = i + 1;
    }
    return h == hashCode(v);
}

function checkBoolHash(bool v, int warmIters): bool {
    Bool w = new Bool(v);
    int h = w.hashCode();
    int i = 0;
    while (i < warmIters) {
        h = w.hashCode();
        i = i + 1;
    }
    return h == hashCode(v);
}

function checkStringHash(string v, int warmIters): bool {
    String w = new String(v);
    int h = w.hashCode();
    int i = 0;
    while (i < warmIters) {
        h = w.hashCode();
        i = i + 1;
    }
    return h == hashCode(v);
}

function checkIntEquals(int a, int b, int warmIters): bool {
    Int wa = new Int(a);
    Int wb = new Int(b);
    bool got = wa.equals(wb);
    int i = 0;
    while (i < warmIters) {
        got = wa.equals(wb);
        i = i + 1;
    }
    return got == (a == b);
}

function checkFloatEquals(float a, float b, int warmIters): bool {
    Float wa = new Float(a);
    Float wb = new Float(b);
    bool got = wa.equals(wb);
    int i = 0;
    while (i < warmIters) {
        got = wa.equals(wb);
        i = i + 1;
    }
    return got == (a == b);
}

function checkBoolEquals(bool a, bool b, int warmIters): bool {
    Bool wa = new Bool(a);
    Bool wb = new Bool(b);
    bool got = wa.equals(wb);
    int i = 0;
    while (i < warmIters) {
        got = wa.equals(wb);
        i = i + 1;
    }
    return got == (a == b);
}

function checkStringEquals(string a, string b, int warmIters): bool {
    String wa = new String(a);
    String wb = new String(b);
    bool got = wa.equals(wb);
    int i = 0;
    while (i < warmIters) {
        got = wa.equals(wb);
        i = i + 1;
    }
    return got == (a == b);
}

function main(): void {
    // 32 warm iterations is well past the JIT/IC promotion threshold;
    // the fast path is active for nearly all calls.
    int warm = 32;
    int fails = 0;

    // ---- Int hashCode ----
    if (!checkIntHash(0, warm))         { fails = fails + 1; }
    if (!checkIntHash(1, warm))         { fails = fails + 1; }
    if (!checkIntHash(-1, warm))        { fails = fails + 1; }
    if (!checkIntHash(2, warm))         { fails = fails + 1; }
    if (!checkIntHash(-2, warm))        { fails = fails + 1; }
    if (!checkIntHash(42, warm))        { fails = fails + 1; }
    if (!checkIntHash(-42, warm))       { fails = fails + 1; }
    if (!checkIntHash(12345, warm))     { fails = fails + 1; }
    if (!checkIntHash(-12345, warm))    { fails = fails + 1; }
    if (!checkIntHash(268435456, warm)) { fails = fails + 1; }
    if (!checkIntHash(-268435456, warm)){ fails = fails + 1; }

    // ---- Float hashCode ----
    if (!checkFloatHash(0.0, warm))     { fails = fails + 1; }
    if (!checkFloatHash(1.0, warm))     { fails = fails + 1; }
    if (!checkFloatHash(-1.0, warm))    { fails = fails + 1; }
    if (!checkFloatHash(0.5, warm))     { fails = fails + 1; }
    if (!checkFloatHash(-0.5, warm))    { fails = fails + 1; }
    if (!checkFloatHash(3.14, warm))    { fails = fails + 1; }
    if (!checkFloatHash(-2.5, warm))    { fails = fails + 1; }

    // ---- Bool hashCode ----
    if (!checkBoolHash(true, warm))     { fails = fails + 1; }
    if (!checkBoolHash(false, warm))    { fails = fails + 1; }

    // ---- String hashCode ----
    if (!checkStringHash("", warm))            { fails = fails + 1; }
    if (!checkStringHash("a", warm))           { fails = fails + 1; }
    if (!checkStringHash("ab", warm))          { fails = fails + 1; }
    if (!checkStringHash("hello", warm))       { fails = fails + 1; }
    if (!checkStringHash("hello world", warm)) { fails = fails + 1; }

    // ---- Int equals ----
    if (!checkIntEquals(0, 0, warm))    { fails = fails + 1; }
    if (!checkIntEquals(1, 1, warm))    { fails = fails + 1; }
    if (!checkIntEquals(-7, -7, warm))  { fails = fails + 1; }
    if (!checkIntEquals(0, 1, warm))    { fails = fails + 1; }
    if (!checkIntEquals(42, -42, warm)) { fails = fails + 1; }
    if (!checkIntEquals(100, 200, warm)){ fails = fails + 1; }

    // ---- Float equals ----
    if (!checkFloatEquals(0.0, 0.0, warm))   { fails = fails + 1; }
    if (!checkFloatEquals(3.14, 3.14, warm)) { fails = fails + 1; }
    if (!checkFloatEquals(0.0, 1.0, warm))   { fails = fails + 1; }
    if (!checkFloatEquals(-2.5, 2.5, warm))  { fails = fails + 1; }

    // ---- Bool equals ----
    if (!checkBoolEquals(true, true, warm))   { fails = fails + 1; }
    if (!checkBoolEquals(false, false, warm)) { fails = fails + 1; }
    if (!checkBoolEquals(true, false, warm))  { fails = fails + 1; }
    if (!checkBoolEquals(false, true, warm))  { fails = fails + 1; }

    // ---- String equals ----
    if (!checkStringEquals("", "", warm))           { fails = fails + 1; }
    if (!checkStringEquals("a", "a", warm))         { fails = fails + 1; }
    if (!checkStringEquals("hello", "hello", warm)) { fails = fails + 1; }
    if (!checkStringEquals("", "a", warm))          { fails = fails + 1; }
    if (!checkStringEquals("a", "b", warm))         { fails = fails + 1; }
    if (!checkStringEquals("hello", "world", warm)) { fails = fails + 1; }

    if (fails == 0) {
        print("PASS");
    } else {
        print("FAILED " + fails);
    }
}
main();
