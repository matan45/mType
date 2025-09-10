// Advanced array operations test

Array<int> numbers = new Array<int>();

// Test adding elements
numbers.add(10);
numbers.add(20);
numbers.add(30);
numbers.add(40);

print(numbers.size()); // 4

// Test bracket notation access
print(numbers[0]); // 10
print(numbers[1]); // 20
print(numbers[2]); // 30
print(numbers[3]); // 40

// Test set operation
numbers.set(1, 99);
print(numbers[1]); // 99

// Test removeAt operation
numbers.removeAt(2); // Remove element at index 2 (was 30)
print(numbers.size()); // 3
print(numbers[0]); // 10
print(numbers[1]); // 99
print(numbers[2]); // 40

// Test clear
numbers.clear();
print(numbers.size());  // 0
print(numbers.empty()); // true

print("Array advanced test passed");