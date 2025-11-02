// Test returning arrays from functions
print("Testing array return values");

int[] createIntArray(int size, int value) {
    int[] arr = new int[size];
    for (int i = 0; i < arr.length; i++) {
        arr[i] = value * (i + 1);
    }
    return arr;
}

int[] result = createIntArray(5, 10);
print("Created array from function:");
for (int i = 0; i < result.length; i++) {
    print("  result[" + i + "] = " + result[i]);
}

string[] createStringArray() {
    string[] arr = new string[3];
    arr[0] = "First";
    arr[1] = "Second";
    arr[2] = "Third";
    return arr;
}

string[] strings = createStringArray();
print("String array from function:");
for (int i = 0; i < strings.length; i++) {
    print("  strings[" + i + "] = " + strings[i]);
}

print("Array return values test completed");
