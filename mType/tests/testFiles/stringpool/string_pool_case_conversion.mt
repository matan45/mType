// Test: toUpperCase / toLowerCase results compare equal to pooled literals
function main(): void {
    string mixed = "Hello World";

    string upper = toUpperCase(mixed);
    print("upper eq literal: " + (upper == "HELLO WORLD"));

    string lower = toLowerCase(mixed);
    print("lower eq literal: " + (lower == "hello world"));

    string twice = toUpperCase(toLowerCase(mixed));
    print("round trip eq upper: " + (twice == "HELLO WORLD"));

    string emptyU = toUpperCase("");
    print("empty upper eq empty: " + (emptyU == ""));

    string allUpper = toUpperCase("ABC");
    print("already upper unchanged: " + (allUpper == "ABC"));
}
main();

// Expected output:
// upper eq literal: true
// lower eq literal: true
// round trip eq upper: true
// empty upper eq empty: true
// already upper unchanged: true
