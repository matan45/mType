// Set iteration test

Set<string> fruits = new Set<string>();

// Add fruits
fruits.add("apple");
fruits.add("banana");
fruits.add("cherry");

print(fruits.size()); // 3

// Iterate through set
for (string fruit : fruits) {
    print(fruit);
}

// Clear the set
fruits.clear();
print(fruits.size());  // 0
print(fruits.empty()); // true

print("Set iteration test passed");