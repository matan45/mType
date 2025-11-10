// Test partially initialized arrays
print("Testing partially initialized arrays");

int[] partial = new int[10];
// Only initialize some elements
partial[0] = 100;
partial[5] = 500;
partial[9] = 900;

print("Partial initialization (only indices 0, 5, 9 set):");
for (int i = 0; i < partial.length; i++) {
    print("  partial[" + i + "] = " + partial[i]);
}

// Sparse pattern
int[] sparse = new int[20];
for (int i = 0; i < sparse.length; i = i + 5) {
    sparse[i] = i * 10;
}

print("Sparse pattern (every 5th element):");
for (int i = 0; i < sparse.length; i++) {
    if (sparse[i] != 0) {
        print("  sparse[" + i + "] = " + sparse[i]);
    }
}

print("Partial initialization test completed");
