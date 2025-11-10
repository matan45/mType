// Test await in array access expression

import { Int } from "../../lib/primitives/Int.mt";

print("=== Await in Array Access Test ===");

function async getArray(): Promise<int[]> {
    print("Creating array");
    int[] arr = new int[3];
    arr[0] = 100;
    arr[1] = 200;
    arr[2] = 300;
    return arr;
}

function async getIndex(): Promise<Int> {
    print("Getting index");
    return new Int(1);
}

function async testArrayAccess(): Promise<Int> {
    print("Testing array access");

    // Await array then access
    int val = (await getArray())[2];
    print("Element at index 2: " + val);

    // Await both array and index
    int[] arr = await getArray();
    Int idx = await getIndex();
    int element = arr[idx.getValue()];
    print("Element at dynamic index: " + element);

    return new Int(element);
}

function async main(): Promise<Int> {
    Int r = await testArrayAccess();
    print("Result: " + r);
    return r;
}

main();
