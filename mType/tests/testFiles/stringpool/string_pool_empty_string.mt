// Test: Empty-string identity and concat with empty
function main(): void {
    string a = "";
    string b = "";
    print("empty eq: " + (a == b));

    string c = a + b;
    print("concat empty eq empty: " + (c == ""));

    string d = "x" + "";
    print("x concat empty eq x: " + (d == "x"));

    string e = "" + "x";
    print("empty concat x eq x: " + (e == "x"));
}
main();

// Expected output:
// empty eq: true
// concat empty eq empty: true
// x concat empty eq x: true
// empty concat x eq x: true
