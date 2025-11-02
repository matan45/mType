// Test storage mode transitions
print("Testing dense to sparse transitions");

// Start dense
int[] dense = new int[50];
for (int i = 0; i < dense.length; i++) {
    dense[i] = i;
}

print("Dense array sum:");
int denseSum = 0;
for (int i = 0; i < dense.length; i++) {
    denseSum = denseSum + dense[i];
}
print("Sum: " + denseSum);

// Create sparse pattern
int[] sparse = new int[100];
sparse[0] = 1;
sparse[25] = 2;
sparse[50] = 3;
sparse[75] = 4;
sparse[99] = 5;

print("Sparse array (5 elements set out of 100):");
int sparseSum = 0;
int sparseCount = 0;
for (int i = 0; i < sparse.length; i++) {
    if (sparse[i] != 0) {
        sparseSum = sparseSum + sparse[i];
        sparseCount = sparseCount + 1;
    }
}
print("Non-zero elements: " + sparseCount);
print("Sum: " + sparseSum);

// Transition: fill sparse array
for (int i = 0; i < sparse.length; i++) {
    if (sparse[i] == 0) {
        sparse[i] = 1;
    }
}

int newSum = 0;
for (int i = 0; i < sparse.length; i++) {
    newSum = newSum + sparse[i];
}
print("After densification, sum: " + newSum);

print("Storage transition test completed");
