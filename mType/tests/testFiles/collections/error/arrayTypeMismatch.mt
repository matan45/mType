// Type mismatch error test

// This should fail: trying to add string to int array
Array<int> numbers = [1, 2, 3];
numbers.add("hello");  // Type mismatch error

print("This should not print");