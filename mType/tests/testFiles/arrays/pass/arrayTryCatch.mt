// Test arrays in exception handling (simulated with conditionals)
print("Testing arrays with error handling");

int[] safeGet(int[] arr, int index) {
    int[] result = new int[2];
    if (index < 0 || index >= arr.length) {
        result[0] = -1;
        result[1] = 0;
        return result;
    }
    result[0] = 0;
    result[1] = arr[index];
    return result;
}

int[] data = new int[5];
for (int i = 0; i < data.length; i++) {
    data[i] = (i + 1) * 10;
}

print("Array: [10, 20, 30, 40, 50]");

// Safe access
int[] result1 = safeGet(data, 2);
if (result1[0] == 0) {
    print("Safe access data[2] = " + result1[1]);
} else {
    print("Error accessing index 2");
}

// Out of bounds access
int[] result2 = safeGet(data, 10);
if (result2[0] == 0) {
    print("Safe access data[10] = " + result2[1]);
} else {
    print("Error accessing index 10 (out of bounds)");
}

// Negative index
int[] result3 = safeGet(data, -1);
if (result3[0] == 0) {
    print("Safe access data[-1] = " + result3[1]);
} else {
    print("Error accessing index -1 (negative)");
}

print("Error handling test completed");
