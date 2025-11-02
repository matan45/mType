// Test ambiguous empty array type
print("Testing ambiguous empty array");

// Empty literal without type context (may cause error)
var ambiguous = [];

print("Array length: " + ambiguous.length);

print("This should not be reached");
