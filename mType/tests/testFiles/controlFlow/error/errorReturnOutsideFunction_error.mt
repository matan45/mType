// Test return statement at script level - should produce error
// Return statements are only valid inside functions or methods

int x = 42;
print("Value: " + x);

// ERROR: return at script level (outside function)
return x;

print("This should not execute");
