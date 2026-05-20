// Test: Non-ASCII strings round-trip and compare equal
function main(): void {
    string accented = "café";
    string accented2 = "café";
    print("accented eq: " + (accented == accented2));

    string symbols = "→←↑↓";
    string symbols2 = "→←↑↓";
    print("symbols eq: " + (symbols == symbols2));

    string mixed = "hello, café! → next";
    string mixedJoin = "hello, " + "café" + "! → next";
    print("mixed eq: " + (mixed == mixedJoin));
}
main();

// Expected output:
// accented eq: true
// symbols eq: true
// mixed eq: true
