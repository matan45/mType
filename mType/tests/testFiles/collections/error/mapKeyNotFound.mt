// Map key not found error test

Map<string, int> scores = new Map<string, int>();
scores.put("Alice", 95);

// This should cause a runtime error
scores.get("Bob"); // Error: Map key not found: Bob