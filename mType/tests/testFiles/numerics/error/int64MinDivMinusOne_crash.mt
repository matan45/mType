// MYT-387: INT64_MIN / -1 currently crashes the whole process with structured
// exception 0xc0000095 (x86 idiv overflow trap) — handleDivInt lacks the
// INT64_MIN/-1 guard that performBinaryOp has. This fixture is registered as
// an explicit SKIP (a crash would abort the entire suite run, so it cannot be
// a normal failing canary). Flip to addOutputVerificationTest with expected
// output -9223372036854775808 once MYT-387 lands.
function main(): void {
    int min = 1 << 63;
    print(min / -1);
}
main();
