// Test arrays in ternary expressions
print("Testing arrays in ternary operator");

int[] evenNumbers = [2, 4, 6, 8, 10];
int[] oddNumbers = [1, 3, 5, 7, 9];

bool useEvens = true;

int[] selected = useEvens ? evenNumbers : oddNumbers;

print("Selected (useEvens = true):");
for (int i = 0; i < selected.length; i++) {
    print("  " + selected[i]);
}

useEvens = false;
selected = useEvens ? evenNumbers : oddNumbers;

print("Selected (useEvens = false):");
for (int i = 0; i < selected.length; i++) {
    print("  " + selected[i]);
}

// Ternary with array elements
int[] data = [10, 20, 30, 40, 50];
int threshold = 25;

for (int i = 0; i < data.length; i++) {
    int category = data[i] > threshold ? 1 : 0;
    print("  data[" + i + "] = " + data[i] + ", category: " + category);
}

print("Ternary operator test completed");
