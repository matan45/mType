// Test filter operation
print("Testing array filter operation");

int[] numbers = new int[10];
for (int i = 0; i < numbers.length; i++) {
    numbers[i] = i + 1;
}

print("Original array: [1, 2, 3, 4, 5, 6, 7, 8, 9, 10]");

// Filter: get even numbers
int evenCount = 0;
for (int i = 0; i < numbers.length; i++) {
    if (numbers[i] % 2 == 0) {
        evenCount = evenCount + 1;
    }
}

int[] evens = new int[evenCount];
int evenIndex = 0;
for (int i = 0; i < numbers.length; i++) {
    if (numbers[i] % 2 == 0) {
        evens[evenIndex] = numbers[i];
        evenIndex = evenIndex + 1;
    }
}

print("Filtered (even numbers):");
for (int i = 0; i < evens.length; i++) {
    print("  " + evens[i]);
}

// Filter: get numbers > 5
int greaterCount = 0;
for (int i = 0; i < numbers.length; i++) {
    if (numbers[i] > 5) {
        greaterCount = greaterCount + 1;
    }
}

int[] greaterThanFive = new int[greaterCount];
int greaterIndex = 0;
for (int i = 0; i < numbers.length; i++) {
    if (numbers[i] > 5) {
        greaterThanFive[greaterIndex] = numbers[i];
        greaterIndex = greaterIndex + 1;
    }
}

print("Filtered (> 5):");
for (int i = 0; i < greaterThanFive.length; i++) {
    print("  " + greaterThanFive[i]);
}

print("Filter operation test completed");
