// Test: switch with a default branch that throws. Specific cases run
// normally; the default-throw is caught by the surrounding try/catch.
import * from "../../lib/exceptions/Exception.mt";

function classify(int code): string {
    string result = "";
    switch (code) {
        case 1:
            result = "one";
            break;
        case 2:
            result = "two";
            break;
        default:
            throw new Exception("unknown code " + code);
    }
    return result;
}

function main(): void {
    print(classify(1));
    print(classify(2));
    try {
        print(classify(99));
    } catch (Exception e) {
        print("caught: " + e.getMessage());
    }
    print(classify(2));
}
main();
