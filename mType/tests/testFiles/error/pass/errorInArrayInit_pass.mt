// Test exception handling in array initializer
import * from "../../lib/exceptions/Exception.mt";

class ArrayException extends Exception {
    constructor(String message) {
        super(message);
    }
}

function getArrayElement(Int index): Int {
    try {
        print("Computing array element " + index);
        if (index == 2) {
            throw new ArrayException("Error at index 2");
        }
        return index * 10;
    } catch (ArrayException e) {
        print("Caught in array init: " + e.getMessage());
        return -1;
    }
}

function main(): void {
    print("Testing exception in array initializer");
    Int[] arr = [getArrayElement(0), getArrayElement(1), getArrayElement(2), getArrayElement(3)];

    print("Array values:");
    for (Int i = 0; i < 4; i = i + 1) {
        print("arr[" + i + "] = " + arr[i]);
    }

    print("Test completed");
}

main();
