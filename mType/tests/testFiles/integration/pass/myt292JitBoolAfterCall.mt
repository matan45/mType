// MYT-292 regression: the JIT's emitLoadLocal must mirror the unboxed
// primitive payload back to stackBase when reloading a bool/int local that
// was stored as a BOXED slot (typical after a call return). Pre-fix,
// JUMP_IF_FALSE on the next iteration read stale stackBase and consistently
// branched the wrong way once OSR tier-up engaged. The demo
// (mtype-imgui-sdl/mt/demo_close.mt) saw `if (helloOpen)` go permanently
// false.
//
// Reproduction requires three things together:
//   1. The hot loop is in `usesBoxedTypes` mode — triggered here by a
//      PUSH_STRING + STORE inside the loop body (the marker assignment).
//      Without it, the call return uses the primitive publish path and the
//      bug doesn't show.
//   2. A method/function call returning a primitive, stored into a local
//      via the boxed STORE_LOCAL path (slot type tagged BOXED).
//   3. The same local read in an `if (bool)` later in the same iteration.

class Helper {
    public static function alwaysTrue():  bool { return true; }
    public static function alwaysFalse(): bool { return false; }
}

string marker = "init";
int sawTrueBranch  = 0;
int sawFalseBranch = 0;
int sawAfterIfs    = 0;
int n = 0;

while (n < 2000) {
    n = n + 1;
    // PUSH_STRING + STORE in the loop body. The outer-scope `marker` is
    // printed after the loop so the optimizer can't eliminate this as a
    // dead store — keeps the JIT in usesBoxedTypes mode.
    marker = "myt292";

    bool tflag = Helper::alwaysTrue();
    bool fflag = Helper::alwaysFalse();

    if (tflag) { sawTrueBranch  = sawTrueBranch  + 1; }
    if (fflag) { sawFalseBranch = sawFalseBranch + 1; }
    sawAfterIfs = sawAfterIfs + 1;
}

print("n=" + n);
print("sawTrueBranch=" + sawTrueBranch);
print("sawFalseBranch=" + sawFalseBranch);
print("sawAfterIfs=" + sawAfterIfs);
print("marker=" + marker);
