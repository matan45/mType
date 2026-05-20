// Test: Concatenation results compare equal to pooled literals
function main(): void {
    string a = "hello";
    string b = "world";
    string joined = a + b;
    print("joined eq literal: " + (joined == "helloworld"));

    string a2 = "hello";
    string b2 = "world";
    string joined2 = a2 + b2;
    print("two joins eq: " + (joined == joined2));

    string greet = "hi" + " " + "there";
    print("triple concat eq literal: " + (greet == "hi there"));
}
main();

// Expected output:
// joined eq literal: true
// two joins eq: true
// triple concat eq literal: true
