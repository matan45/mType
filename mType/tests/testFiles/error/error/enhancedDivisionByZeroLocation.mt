// Test enhanced division by zero exception with location info
int x = 10;
int y = 0;
int result = x / y;  // Line 4: Should show location in error message
print("This should not execute");