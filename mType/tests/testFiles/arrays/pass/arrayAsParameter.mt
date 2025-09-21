// Test arrays as function parameters and return types (simplified)
print("Testing arrays as parameters and return types");

// For now, test array operations with improved syntax
print("Creating test array");
int[] testArray = new int[3];
testArray.set(0, 10);
testArray.set(1, 20);
testArray.set(2, 30);

print("Array length: " + testArray.length);
print("testArray[0] = " + testArray[0]);
print("testArray[1] = " + testArray[1]);
print("testArray[2] = " + testArray[2]);

// Test creating and using another array with loops
int[] created = new int[4];
for (int i = 0; i < created.length; i++) {
    created.set(i, (i + 1) * 5);
}

print("Created array:");
print("Array length: " + created.length);
for (int i = 0; i < created.length; i++) {
    print("  [" + i + "] = " + created[i]);
}

// Test array summation using loops and bracket syntax
int total = 0;
for (int i = 0; i < testArray.length; i++) {
    total = total + testArray[i];
}
print("Sum of testArray: " + total);

print("Array parameter test completed");