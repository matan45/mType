// Map literal syntax

// Create map with literal syntax
Map<string, int> ages = {"Alice": 30, "Bob": 25, "Charlie": 35};

// Test size
print(ages.size());  // 3

// Access values
print(ages.get("Alice"));    // 30
print(ages.get("Bob"));      // 25
print(ages.get("Charlie"));  // 35

// Create map with different types
Map<string, string> colors = {"red": "#FF0000", "green": "#00FF00", "blue": "#0000FF"};
print(colors.size());        // 3
print(colors.get("red"));    // #FF0000
print(colors.get("blue"));   // #0000FF

print("Map literal test passed");