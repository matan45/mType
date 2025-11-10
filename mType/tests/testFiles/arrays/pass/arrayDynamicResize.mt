// Test resizing arrays dynamically
print("Testing dynamic array resizing");

int[] original = new int[3];
original[0] = 10;
original[1] = 20;
original[2] = 30;

print("Original array (size 3):");
for (int i = 0; i < original.length; i++) {
    print("  original[" + i + "] = " + original[i]);
}

// Resize to larger array
int[] resized = new int[5];
for (int i = 0; i < original.length; i++) {
    resized[i] = original[i];
}
resized[3] = 40;
resized[4] = 50;

print("Resized array (size 5):");
for (int i = 0; i < resized.length; i++) {
    print("  resized[" + i + "] = " + resized[i]);
}

// Resize to smaller array
int[] shrunk = new int[2];
for (int i = 0; i < shrunk.length; i++) {
    shrunk[i] = resized[i];
}

print("Shrunk array (size 2):");
for (int i = 0; i < shrunk.length; i++) {
    print("  shrunk[" + i + "] = " + shrunk[i]);
}

print("Dynamic resize test completed");
