// Test basic array literal syntax for different types
print("Testing basic array literals");

// Integer arrays
int[] numbers = [1, 2, 3, 4, 5];
print("Integer array length: " + numbers.length);
for (int i = 0; i < numbers.length; i++) {
    print("numbers[" + i + "] = " + numbers[i]);
}

// String arrays
string[] names = ["Alice", "Bob", "Charlie"];
print("String array length: " + names.length);
for (int i = 0; i < names.length; i++) {
    print("names[" + i + "] = " + names[i]);
}

// Float arrays
float[] prices = [10.99, 20.50, 15.25];
print("Float array length: " + prices.length);
for (int i = 0; i < prices.length; i++) {
    print("prices[" + i + "] = " + prices[i]);
}

// Boolean arrays
bool[] flags = [true, false, true, false];
print("Boolean array length: " + flags.length);
for (int i = 0; i < flags.length; i++) {
    print("flags[" + i + "] = " + flags[i]);
}

// Empty arrays
int[] empty = [];
print("Empty array length: " + empty.length);

print("Basic array literals test completed");