// Array literal syntax

// Create array with literal syntax
Array<int> numbers = [1, 2, 3, 4, 5];

// Test size
print(numbers.size());  // 5

// Print all elements
for (int i = 0; i < numbers.size(); i = i + 1) {
    print(numbers.get(i));
}

// Create string array
Array<string> names = ["Alice", "Bob", "Charlie"];
print(names.size());  // 3
print(names.get(0));  // Alice
print(names.get(2));  // Charlie

print("Array literal test passed");