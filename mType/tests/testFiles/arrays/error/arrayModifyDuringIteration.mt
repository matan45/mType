// Test modification during iteration (unsafe pattern)
print("Testing modification during iteration");

int[] numbers = new int[5];
for (int i = 0; i < numbers.length; i++) {
    numbers[i] = i;
}

// Attempt to modify array while iterating
// This pattern is unsafe and should be avoided
for (int i = 0; i < numbers.length; i++) {
    numbers[i] = numbers[i] * 2;
    // Trying to access beyond bounds during modification
    if (i < numbers.length - 1) {
        // Force out of bounds by modifying during iteration
        int temp = numbers[numbers.length + i];
    }
}

print("This should not be reached");
