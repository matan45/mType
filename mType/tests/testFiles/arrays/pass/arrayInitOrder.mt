// Test initialization ordering
print("Testing initialization ordering");

int orderCounter = 0;

int getNext() {
    orderCounter = orderCounter + 1;
    print("  getNext() called, returning " + orderCounter);
    return orderCounter;
}

print("Creating array with function calls:");
int[] ordered = [getNext(), getNext(), getNext(), getNext()];

print("Array values:");
for (int i = 0; i < ordered.length; i++) {
    print("  ordered[" + i + "] = " + ordered[i]);
}

// Nested array initialization order
orderCounter = 0;
print("Creating nested array:");
int[][] nested = [
    [getNext(), getNext()],
    [getNext(), getNext()],
    [getNext(), getNext()]
];

print("Nested array values:");
for (int i = 0; i < nested.length; i++) {
    for (int j = 0; j < nested[i].length; j++) {
        print("  nested[" + i + "][" + j + "] = " + nested[i][j]);
    }
}

print("Initialization order test completed");
