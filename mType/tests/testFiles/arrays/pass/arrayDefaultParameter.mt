// Test arrays as default parameters (simulated with overloading pattern)
print("Testing arrays as default parameters");

int[] getDefaultArray() {
    int[] arr = new int[3];
    arr[0] = 1;
    arr[1] = 2;
    arr[2] = 3;
    return arr;
}

int sumArray(int[] arr) {
    int total = 0;
    for (int i = 0; i < arr.length; i++) {
        total = total + arr[i];
    }
    return total;
}

// Call with custom array
int[] custom = [10, 20, 30, 40];
int customSum = sumArray(custom);
print("Sum of custom array: " + customSum);

// Call with default array
int[] defaultArr = getDefaultArray();
int defaultSum = sumArray(defaultArr);
print("Sum of default array: " + defaultSum);

// Function using default pattern
void processArray(int[] arr) {
    print("Processing array of length: " + arr.length);
    for (int i = 0; i < arr.length; i++) {
        print("  Element " + i + ": " + arr[i]);
    }
}

processArray(custom);
processArray(getDefaultArray());

print("Default parameter test completed");
