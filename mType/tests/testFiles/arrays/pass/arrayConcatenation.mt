// Test concatenating arrays
print("Testing array concatenation");

int[] arr1 = new int[3];
arr1[0] = 1;
arr1[1] = 2;
arr1[2] = 3;

int[] arr2 = new int[2];
arr2[0] = 4;
arr2[1] = 5;

print("Array 1:");
for (int i = 0; i < arr1.length; i++) {
    print("  arr1[" + i + "] = " + arr1[i]);
}

print("Array 2:");
for (int i = 0; i < arr2.length; i++) {
    print("  arr2[" + i + "] = " + arr2[i]);
}

// Concatenate arrays
int[] concatenated = new int[arr1.length + arr2.length];
for (int i = 0; i < arr1.length; i++) {
    concatenated[i] = arr1[i];
}
for (int i = 0; i < arr2.length; i++) {
    concatenated[arr1.length + i] = arr2[i];
}

print("Concatenated array:");
for (int i = 0; i < concatenated.length; i++) {
    print("  concatenated[" + i + "] = " + concatenated[i]);
}

// Concatenate with empty array
int[] empty = new int[0];
int[] withEmpty = new int[arr1.length + empty.length];
for (int i = 0; i < arr1.length; i++) {
    withEmpty[i] = arr1[i];
}

print("Concatenated with empty array, length: " + withEmpty.length);

print("Array concatenation test completed");
