// Function with return in try, in catch, and in finally. No exception is thrown,
// so the catch never executes. Finally's return must take precedence over try's.
// Expected result: C (the finally return).
import * from "../../lib/exceptions/Exception.mt";

function whichReturns(): string {
    try {
        print("Try block (about to return A)");
        return "A";
    } catch (Exception e) {
        print("Catch block (about to return B)");
        return "B";
    } finally {
        print("Finally block (about to return C)");
        return "C";
    }
}

function main(): void {
    print("Testing return precedence: try vs catch vs finally");
    string result = whichReturns();
    print("Observed: " + result);

    if (result == "C") {
        print("Outcome: finally won");
    } else {
        print("Outcome: " + result + " won (unexpected)");
    }

    print("Return precedence test completed");
}

main();
