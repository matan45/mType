// Test copy-on-write semantics
print("Testing copy-on-write semantics");

int[] original = new int[4];
for (int i = 0; i < original.length; i++) {
    original[i] = (i + 1) * 10;
}

print("Original array:");
for (int i = 0; i < original.length; i++) {
    print("  " + original[i]);
}

// Create independent copy
int[] copy = new int[original.length];
for (int i = 0; i < original.length; i++) {
    copy[i] = original[i];
}

// Modify copy
copy[0] = 999;
copy[2] = 888;

print("After modifying copy:");
print("Original[0] = " + original[0]);
print("Copy[0] = " + copy[0]);
print("Original[2] = " + original[2]);
print("Copy[2] = " + copy[2]);

// Modify original
original[1] = 777;

print("After modifying original:");
print("Original[1] = " + original[1]);
print("Copy[1] = " + copy[1]);

print("Copy-on-write test completed");
