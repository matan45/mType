// finally-return must override try-return: result is 2.
import * from "../../lib/exceptions/Exception.mt";

function pickReturn(): int {
    try {
        print("Try block");
        return 1;
    } finally {
        print("Finally block");
        return 2;
    }
}

function main(): void {
    print("Testing finally return overriding try return");
    int result = pickReturn();
    print("Observed result: " + result);

    if (result == 2) {
        print("Outcome: finally won");
    } else {
        print("Outcome: try won (unexpected)");
    }

    print("Finally-overrides-try-return test completed");
}

main();
