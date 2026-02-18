// Test: Primitive types cannot be nullable

// This should cause a parse error: int cannot be nullable
int? x = 5;
print("Should not reach here");
