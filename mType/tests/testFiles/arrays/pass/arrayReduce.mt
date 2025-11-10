// Test reduce/fold operation
print("Testing array reduce operation");

int[] numbers = new int[5];
numbers[0] = 1;
numbers[1] = 2;
numbers[2] = 3;
numbers[3] = 4;
numbers[4] = 5;

print("Array: [1, 2, 3, 4, 5]");

// Reduce: sum
int sum = 0;
for (int i = 0; i < numbers.length; i++) {
    sum = sum + numbers[i];
}
print("Sum (reduce with +): " + sum);

// Reduce: product
int product = 1;
for (int i = 0; i < numbers.length; i++) {
    product = product * numbers[i];
}
print("Product (reduce with *): " + product);

// Reduce: max
int max = numbers[0];
for (int i = 1; i < numbers.length; i++) {
    if (numbers[i] > max) {
        max = numbers[i];
    }
}
print("Max (reduce with max): " + max);

// Reduce: min
int min = numbers[0];
for (int i = 1; i < numbers.length; i++) {
    if (numbers[i] < min) {
        min = numbers[i];
    }
}
print("Min (reduce with min): " + min);

print("Reduce operation test completed");
