// Test exception handling in switch case expression
import * from "../../lib/exceptions/Exception.mt";

class SwitchException extends Exception {
    constructor(String message) {
        super(message);
    }
}

function computeCaseValue(Int caseNum): Int {
    try {
        print("Computing case " + caseNum + " expression");
        if (caseNum == 2) {
            throw new SwitchException("Error in case 2");
        }
        return caseNum * 10;
    } catch (SwitchException e) {
        print("Caught in switch case: " + e.getMessage());
        return -1;
    }
}

function main(): void {
    print("Testing exception in switch case expression");

    for (Int i = 0; i < 4; i = i + 1) {
        Int value = computeCaseValue(i);

        switch (value) {
            case 0:
                print("Case 0 matched");
                break;
            case 10:
                print("Case 1 matched");
                break;
            case -1:
                print("Exception case matched");
                break;
            case 30:
                print("Case 3 matched");
                break;
            default:
                print("Default case");
                break;
        }
    }

    print("Test completed");
}

main();
