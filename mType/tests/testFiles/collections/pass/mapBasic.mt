// Basic map creation and usage

// Create an empty map
Map<string, int> ages = new Map<string, int>();

// Add key-value pairs
ages.put("Alice", 30);
ages.put("Bob", 25);
ages.put("Charlie", 35);

// Print map size
print(ages.size());  // 3

// Get values by key
print(ages.get("Alice"));    // 30
print(ages.get("Bob"));      // 25
print(ages.get("Charlie"));  // 35

// Check if key exists
print(ages.containsKey("Alice"));   // true
print(ages.containsKey("David"));   // false

print("Map basic test passed");