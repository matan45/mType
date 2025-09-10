// Advanced map operations test

Map<string, string> countries = new Map<string, string>();

// Add key-value pairs
countries.put("USA", "Washington DC");
countries.put("UK", "London");
countries.put("France", "Paris");
countries.put("Germany", "Berlin");

print(countries.size()); // 4

// Test keySet functionality
Array<string> keys = countries.keySet();
print(keys.size()); // 4

// Iterate through keys
for (string key : keys) {
    print(key + ": " + countries.get(key));
}

// Test overwrite existing key
countries.put("USA", "New York"); // Overwrites previous value
print(countries.get("USA")); // "New York"
print(countries.size());     // 4 (size unchanged)

// Test remove
countries.remove("Germany");
print(countries.size());            // 3
print(countries.containsKey("Germany")); // false

// Test clear
countries.clear();
print(countries.empty()); // true
print(countries.size());  // 0

print("Map advanced test passed");