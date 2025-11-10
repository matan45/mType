// Test linear search indexOf operation
print("Testing array linear search");

int[] numbers = new int[10];
for (int i = 0; i < numbers.length; i++) {
    numbers[i] = i * 5;
}

function findIndex(int[] arr, int target): int {
    for (int i = 0; i < arr.length; i++) {
        if (arr[i] == target) {
            return i;
        }
    }
    return -1;
}

print("Array: [0, 5, 10, 15, 20, 25, 30, 35, 40, 45]");
print("indexOf(20) = " + findIndex(numbers, 20));
print("indexOf(45) = " + findIndex(numbers, 45));
print("indexOf(0) = " + findIndex(numbers, 0));
print("indexOf(99) = " + findIndex(numbers, 99));

// Test with string array
string[] names = new string[5];
names[0] = "Alice";
names[1] = "Bob";
names[2] = "Charlie";
names[3] = "David";
names[4] = "Eve";

function findStringIndex(string[] arr, string target): int {
    for (int i = 0; i < arr.length; i++) {
        if (arr[i] == target) {
            return i;
        }
    }
    return -1;
}

print("String array search:");
print("indexOf(Charlie) = " + findStringIndex(names, "Charlie"));
print("indexOf(Alice) = " + findStringIndex(names, "Alice"));
print("indexOf(Frank) = " + findStringIndex(names, "Frank"));

print("Linear search test completed");
