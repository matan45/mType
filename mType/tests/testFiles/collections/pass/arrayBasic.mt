// Basic array creation and usage

// Create an empty array
Array<int> numbers = new Array<int>();

// Add elements to the array
numbers.add(10);
numbers.add(20);
numbers.add(30);

// Print array size
print(numbers.size());  // 3

// Print elements
print(numbers.get(0));  // 10
print(numbers.get(1));  // 20
print(numbers.get(2));  // 30

// Test array modification
numbers.set(1, 99);
print(numbers.get(1));  // 99

print("Array basic test passed");