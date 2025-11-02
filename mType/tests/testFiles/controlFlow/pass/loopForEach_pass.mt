// Test foreach/enhanced for loops using arrays

// Simulate foreach with indexed for loop on array
int[] numbers = [1, 2, 3, 4, 5];
for (int i = 0; i < 5; i++) {
    print(numbers[i]);
}
print("Array iteration complete");

// Iterate and sum array elements
int sum = 0;
for (int i = 0; i < 5; i++) {
    sum = sum + numbers[i];
}
print(sum);

// Iterate array with early exit
int[] values = [10, 20, 30, 40, 50];
int target = 30;
bool found = false;
for (int i = 0; i < 5; i++) {
    if (values[i] == target) {
        print("Found at index:");
        print(i);
        found = true;
        break;
    }
}

// Iterate and modify array
int[] data = [1, 2, 3, 4, 5];
for (int i = 0; i < 5; i++) {
    data[i] = data[i] * 2;
}
for (int i = 0; i < 5; i++) {
    print(data[i]);
}

// Nested array iteration
int[][] matrix = [[1, 2], [3, 4]];
for (int i = 0; i < 2; i++) {
    for (int j = 0; j < 2; j++) {
        print(matrix[i][j]);
    }
}

print("Test passed");
