// Basic set creation and usage

// Create an empty set
Set<int> numbers = new Set<int>();

// Add elements to the set
print(numbers.add(10));  // true
print(numbers.add(20));  // true
print(numbers.add(30));  // true
print(numbers.add(10));  // false (already exists)

// Print set size
print(numbers.size());  // 3

// Check if elements exist
print(numbers.contains(10));  // true
print(numbers.contains(20));  // true
print(numbers.contains(40));  // false

// Remove elements
print(numbers.remove(20));  // true
print(numbers.remove(40));  // false (doesn't exist)
print(numbers.size());      // 2

print("Set basic test passed");