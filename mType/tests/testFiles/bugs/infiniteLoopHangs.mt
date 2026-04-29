// MYT-227: a regular for/while loop in the JIT lowers its back-edge to a
// tight cc.jmp with no interrupt poll, so an infinite loop hangs forever
// with no diagnostic — Ctrl-C is the only exit. Sibling to MYT-226
// (which is the tail-recursive variant of the same underlying issue).
//
// EXPECTED (one of):
//   - cooperative interrupt poll at the back-edge so external cancellation
//     can stop the loop; OR
//   - configurable per-VM iteration budget that throws RuntimeException
//     when exceeded.
//
// ACTUAL (broken):
//   process hangs forever.

while (true) {
}

print("unreachable");
