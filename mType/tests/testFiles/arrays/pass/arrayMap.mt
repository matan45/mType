// Test map transformation operation
print("Testing array map operation");

int[] numbers = new int[5];
for (int i = 0; i < numbers.length; i++) {
    numbers[i] = i + 1;
}

print("Original array:");
for (int i = 0; i < numbers.length; i++) {
    print("  " + numbers[i]);
}

// Map: multiply each element by 2
int[] doubled = new int[numbers.length];
for (int i = 0; i < numbers.length; i++) {
    doubled[i] = numbers[i] * 2;
}

print("Doubled (map x * 2):");
for (int i = 0; i < doubled.length; i++) {
    print("  " + doubled[i]);
}

// Map: square each element
int[] squared = new int[numbers.length];
for (int i = 0; i < numbers.length; i++) {
    squared[i] = numbers[i] * numbers[i];
}

print("Squared (map x * x):");
for (int i = 0; i < squared.length; i++) {
    print("  " + squared[i]);
}

print("Map operation test completed");
