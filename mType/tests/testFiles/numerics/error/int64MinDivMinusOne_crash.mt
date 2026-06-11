// Historical MYT-387 crash repro: INT64_MIN / -1 used to terminate the process
// with structured exception 0xc0000095 from x86 idiv overflow. Active coverage
// now lives in numerics/pass/int64MinDivMinusOne.mt.
function main(): void {
    int min = 1 << 63;
    print(min / -1);
}
main();
