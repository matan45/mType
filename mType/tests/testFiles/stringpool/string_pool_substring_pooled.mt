// Test: substring results compare equal regardless of source string
function main(): void {
    string source = "abcdefgh";
    string mid1 = substring(source, 2, 3);
    string mid2 = substring("abcdefgh", 2, 3);
    print("mid eq: " + (mid1 == mid2));
    print("mid eq literal: " + (mid1 == "cde"));

    string head = substring(source, 0, 4);
    print("head eq abcd: " + (head == "abcd"));

    string tail = substring(source, 5, 3);
    print("tail eq fgh: " + (tail == "fgh"));

    string zero = substring(source, 2, 0);
    print("zero len eq empty: " + (zero == ""));
}
main();

// Expected output:
// mid eq: true
// mid eq literal: true
// head eq abcd: true
// tail eq fgh: true
// zero len eq empty: true
