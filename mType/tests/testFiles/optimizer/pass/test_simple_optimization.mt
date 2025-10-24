// Simple test to see optimization in action

// Test 1: Constant folding - should become PUSH_INT 30
int result1 = 10 + 20;
print(result1);

// Test 2: Algebraic simplification - x + 0 should become just x
int x = 42;
int result2 = x + 0;
print(result2);

// Test 3: Dead code - unused expression should be removed
5 * 10;  // This does nothing, should be optimized away

// Test 4: Strength reduction - x / 1 should become just x
int result3 = x / 1;
print(result3);

// Test 5: Comparison folding - should become PUSH_BOOL true
bool result4 = 10 < 20;
print(result4);

// Test 6: Multiple constant operations
int result5 = (10 + 5) * 2;
print(result5);

print("Optimization test completed");
