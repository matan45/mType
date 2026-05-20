// Test: Long strings (> 256 chars) compare equal regardless of construction path
function main(): void {
    string base = "abcdefghij";
    string built = "";
    for (int i = 0; i < 32; i++) {
        built = built + base;
    }
    print("built length: " + strLength(built));

    string built2 = "";
    for (int i = 0; i < 32; i++) {
        built2 = built2 + "abcdefghij";
    }
    print("two builds eq: " + (built == built2));

    string head = substring(built, 0, 10);
    print("head eq base: " + (head == base));
}
main();

// Expected output:
// built length: 320
// two builds eq: true
// head eq base: true
