// Test: INT64_MIN % -1 is defined as 0 (Java semantics), not a hardware trap.
// The division twin (INT64_MIN / -1) currently crashes the process — MYT-387.
function main(): void {
    int min = 1 << 63;
    print(min % -1);
}
main();

// Expected output:
// 0
