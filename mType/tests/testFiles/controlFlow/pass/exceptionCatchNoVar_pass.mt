// Test catch without exception variable binding
import * from "../../lib/exceptions/Exception.mt";

function testCatchNoVar(): void {
    try {
        print("Throwing exception");
        throw new Exception("Test message");
    } catch (Exception) {
        print("Caught exception without binding");
    }
}

function testMultipleCatchNoVar(int testCase): void {
    try {
        if (testCase == 1) {
            throw new Exception("Case 1");
        } else {
            String s = null;
            print(s.length());
        }
    } catch (Exception) {
        print("Caught generic exception");
    }
}

function main(): void {
    print("Testing catch without variable binding");

    testCatchNoVar();
    testMultipleCatchNoVar(1);
    testMultipleCatchNoVar(2);

    print("Test completed");
}

main();
