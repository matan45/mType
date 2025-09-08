// Set type mismatch error test

Set<int> numbers = new Set<int>();

// This should cause a type error
numbers.add("string"); // Error: expected int but got string