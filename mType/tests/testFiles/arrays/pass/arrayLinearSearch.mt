// Test linear search indexOf operation
print("Testing array linear search");

int[] numbers = new int[10];
for (int i = 0; i < numbers.length; i++) {
    numbers[i] = i * 5;
}

int indexOf(int[] arr, int target) {
    for (int i = 0; i < arr.length; i++) {
        if (arr[i] == target) {
            return i;
        }
    }
    return -1;
}

print("Array: [0, 5, 10, 15, 20, 25, 30, 35, 40, 45]");
print("indexOf(20) = " + indexOf(numbers, 20));
print("indexOf(45) = " + indexOf(numbers, 45));
print("indexOf(0) = " + indexOf(numbers, 0));
print("indexOf(99) = " + indexOf(numbers, 99));

// Test with string array
string[] names = new string[5];
names[0] = "Alice";
names[1] = "Bob";
names[2] = "Charlie";
names[3] = "David";
names[4] = "Eve";

int indexOfString(string[] arr, string target) {
    for (int i = 0; i < arr.length; i++) {
        if (arr[i] == target) {
            return i;
        }
    }
    return -1;
}

print("String array search:");
print("indexOf(Charlie) = " + indexOfString(names, "Charlie"));
print("indexOf(Alice) = " + indexOfString(names, "Alice"));
print("indexOf(Frank) = " + indexOfString(names, "Frank"));

print("Linear search test completed");
