// Test dense to sparse storage transitions
print("Testing storage optimization transitions");

// Start with dense array
int[] dense = new int[20];
for (int i = 0; i < dense.length; i++) {
    dense[i] = i;
}

print("Dense array (all elements set):");
print("First 5 elements:");
for (int i = 0; i < 5; i++) {
    print("  dense[" + i + "] = " + dense[i]);
}

// Create sparse pattern (only some elements used)
int[] sparse = new int[100];
sparse[0] = 1;
sparse[10] = 2;
sparse[50] = 3;
sparse[99] = 4;

print("Sparse array (few elements set):");
print("  sparse[0] = " + sparse[0]);
print("  sparse[10] = " + sparse[10]);
print("  sparse[50] = " + sparse[50]);
print("  sparse[99] = " + sparse[99]);

// Transition: make sparse array dense
for (int i = 0; i < sparse.length; i++) {
    if (sparse[i] == 0) {
        sparse[i] = i + 1;
    }
}

print("After densification, first 5 elements:");
for (int i = 0; i < 5; i++) {
    print("  sparse[" + i + "] = " + sparse[i]);
}

print("Optimization transition test completed");
