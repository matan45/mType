// Test async functions with array return types

print("=== Async With Arrays Test ===");

// Async function returning Promise<int[]>
// Implementation returns raw int[], bytecode wraps it
function async getNumbers(): Promise<int[]> {
    int[] numbers = new int[3];
    numbers[0] = 10;
    numbers[1] = 20;
    numbers[2] = 30;
    return numbers;
}

// Test awaiting array promise
function async processNumbers(): Promise<int[]> {
    // await unwraps Promise<int[]> to int[]
    int[] arr = await getNumbers();

    // Modify array
    arr[1] = 25;

    return arr;  // Bytecode wraps in Promise
}

// Main function to run test
function async main(): Promise<int[]> {
    // await unwraps Promise<int[]> to int[]
    int[] finalArray = await processNumbers();

    print("Array[0]: " + finalArray[0]);
    print("Array[1]: " + finalArray[1]);
    print("Array[2]: " + finalArray[2]);
    return finalArray;
}

main();
