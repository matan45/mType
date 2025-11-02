// Test contains operation
print("Testing array contains operation");

int[] numbers = new int[6];
numbers[0] = 10;
numbers[1] = 20;
numbers[2] = 30;
numbers[3] = 40;
numbers[4] = 50;
numbers[5] = 60;

bool contains(int[] arr, int value) {
    for (int i = 0; i < arr.length; i++) {
        if (arr[i] == value) {
            return true;
        }
    }
    return false;
}

print("Array: [10, 20, 30, 40, 50, 60]");
print("contains(30): " + contains(numbers, 30));
print("contains(10): " + contains(numbers, 10));
print("contains(60): " + contains(numbers, 60));
print("contains(25): " + contains(numbers, 25));
print("contains(100): " + contains(numbers, 100));

// Test with empty array
int[] empty = new int[0];
print("Empty array contains(10): " + contains(empty, 10));

print("Contains operation test completed");
