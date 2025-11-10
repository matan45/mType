// Test array slicing operations (manual slicing)
print("Testing array slicing");

int[] source = new int[10];
for (int i = 0; i < source.length; i++) {
    source[i] = i * 10;
}

print("Source array:");
for (int i = 0; i < source.length; i++) {
    print("  source[" + i + "] = " + source[i]);
}

// Manual slice [2:5] (indices 2, 3, 4)
int[] slice1 = new int[3];
for (int i = 0; i < 3; i++) {
    slice1[i] = source[i + 2];
}

print("Slice [2:5]:");
for (int i = 0; i < slice1.length; i++) {
    print("  slice1[" + i + "] = " + slice1[i]);
}

// Manual slice [0:3]
int[] slice2 = new int[3];
for (int i = 0; i < 3; i++) {
    slice2[i] = source[i];
}

print("Slice [0:3]:");
for (int i = 0; i < slice2.length; i++) {
    print("  slice2[" + i + "] = " + slice2[i]);
}

// Manual slice [7:10]
int[] slice3 = new int[3];
for (int i = 0; i < 3; i++) {
    slice3[i] = source[i + 7];
}

print("Slice [7:10]:");
for (int i = 0; i < slice3.length; i++) {
    print("  slice3[" + i + "] = " + slice3[i]);
}

print("Array slicing test completed");
