// Test lastIndexOf operation
print("Testing array lastIndexOf operation");

int[] numbers = new int[10];
numbers[0] = 5;
numbers[1] = 10;
numbers[2] = 5;
numbers[3] = 20;
numbers[4] = 5;
numbers[5] = 30;
numbers[6] = 40;
numbers[7] = 5;
numbers[8] = 50;
numbers[9] = 5;

int lastIndexOf(int[] arr, int value) {
    for (int i = arr.length - 1; i >= 0; i = i - 1) {
        if (arr[i] == value) {
            return i;
        }
    }
    return -1;
}

print("Array: [5, 10, 5, 20, 5, 30, 40, 5, 50, 5]");
print("lastIndexOf(5): " + lastIndexOf(numbers, 5));
print("lastIndexOf(10): " + lastIndexOf(numbers, 10));
print("lastIndexOf(50): " + lastIndexOf(numbers, 50));
print("lastIndexOf(99): " + lastIndexOf(numbers, 99));

// Test with unique elements
int[] unique = new int[3];
unique[0] = 1;
unique[1] = 2;
unique[2] = 3;

print("Unique array [1, 2, 3]:");
print("lastIndexOf(2): " + lastIndexOf(unique, 2));

print("LastIndexOf operation test completed");
