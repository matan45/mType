final int CONSTANT = 42;
print(CONSTANT);  // Should print 42

// This should throw an error: Cannot modify final variable
CONSTANT = 50;  // ERROR!
print(CONSTANT);  // Should not reach here