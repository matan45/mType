// Test break and continue effects in array iteration
print("Testing early termination in loops");

int[] numbers = new int[10];
for (int i = 0; i < numbers.length; i++) {
    numbers[i] = i + 1;
}

// Test break
print("Testing break at 5:");
for (int i = 0; i < numbers.length; i++) {
    if (numbers[i] == 5) {
        print("  Breaking at " + numbers[i]);
        break;
    }
    print("  Processing " + numbers[i]);
}

// Test continue
print("Testing continue (skip evens):");
for (int i = 0; i < numbers.length; i++) {
    if (numbers[i] % 2 == 0) {
        continue;
    }
    print("  Odd number: " + numbers[i]);
}

// Test break in nested loop
print("Testing break in nested loop:");
int[][] matrix = new int[3][];
for (int i = 0; i < matrix.length; i++) {
    matrix[i] = new int[3];
    for (int j = 0; j < matrix[i].length; j++) {
        matrix[i][j] = i * 3 + j;
    }
}

for (int i = 0; i < matrix.length; i++) {
    for (int j = 0; j < matrix[i].length; j++) {
        if (matrix[i][j] == 5) {
            print("  Found 5, breaking inner loop");
            break;
        }
        print("  matrix[" + i + "][" + j + "] = " + matrix[i][j]);
    }
}

print("Early termination test completed");
