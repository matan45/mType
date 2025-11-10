// Test arrays in tight loops (1000 iterations)
print("Testing tight loop performance");

int[] data = new int[10];
for (int i = 0; i < data.length; i++) {
    data[i] = i;
}

print("Initial array sum:");
int sum = 0;
for (int i = 0; i < data.length; i++) {
    sum = sum + data[i];
}
print("Sum: " + sum);

// Tight loop: 1000 iterations of array operations
int iterations = 1000;
for (int iter = 0; iter < iterations; iter++) {
    // Increment each element
    for (int i = 0; i < data.length; i++) {
        data[i] = data[i] + 1;
    }
}

print("After " + iterations + " iterations:");
int finalSum = 0;
for (int i = 0; i < data.length; i++) {
    finalSum = finalSum + data[i];
}
print("Final sum: " + finalSum);
print("Expected sum: " + (sum + (iterations * data.length)));

print("Tight loop test completed");
