// Final variables with different types
final int MAX_SIZE = 100;
final float PI = 3.14159;
final bool IS_ENABLED = true;
final string APP_NAME = "MyApp";

// Regular variables for comparison
int size = 50;
float radius = 2.5;

// Use final variables in calculations
float circumference = 2.0 * PI * radius;
print(circumference);  // Should work fine

// Modify regular variable
size = 75;
print(size);  // Should print 75

// Use final variable
print(MAX_SIZE);  // Should print 100
print(IS_ENABLED); // Should print 1 (true)