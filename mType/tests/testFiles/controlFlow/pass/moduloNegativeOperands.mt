// Test modulo operations with negative operands
// This tests various combinations of positive and negative numbers with modulo

// Test 1: Positive % Positive (baseline)
int result1 = 10 % 3;
print(result1);  // Should be 1

// Test 2: Negative % Positive
int result2 = -10 % 3;
print(result2);  // Behavior depends on implementation

// Test 3: Positive % Negative  
int result3 = 10 % -3;
print(result3);  // Behavior depends on implementation

// Test 4: Negative % Negative
int result4 = -10 % -3;
print(result4);  // Behavior depends on implementation

// Test 5: Edge case - zero results
int result5 = -6 % 3;
print(result5);  // Should be 0

int result6 = 6 % -3;
print(result6);  // Should be 0

// Test 6: Modulo with 1 and -1
int result7 = -5 % 1;
print(result7);  // Should be 0

int result8 = -5 % -1;
print(result8);  // Should be 0

int result9 = 5 % -1;
print(result9);  // Should be 0

// Test 7: Larger negative numbers
int result10 = -17 % 5;
print(result10);

int result11 = 17 % -5;
print(result11);

int result12 = -17 % -5;
print(result12);

// Test 8: Test with variables
int negativeNum = -13;
int positiveNum = 4;

int result13 = negativeNum % positiveNum;
print(result13);

int result14 = positiveNum % negativeNum;
print(result14);

// Test 9: Compound assignment with modulo and negative numbers
int compound1 = -15;
compound1 %= 4;
print(compound1);

int compound2 = 15;
compound2 %= -4;
print(compound2);

// Test 10: Modulo in expressions with negative numbers
int expr1 = (-8 % 3) + (8 % -3);
print(expr1);

int expr2 = (-8 % -3) - (8 % 3);
print(expr2);

print("Modulo negative operands test completed");