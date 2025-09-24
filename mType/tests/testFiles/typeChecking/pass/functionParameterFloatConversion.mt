function processFloat(float f): float {
    return f * 2.0;
}

// Test that int can be passed to float parameter
float result1 = processFloat(5);      // int to float conversion
float result2 = processFloat(3.14);   // direct float

print(result1);
print(result2);