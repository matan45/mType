// Final variable outside loop - this makes sense
final int LIMIT = 5;

// Regular counter
int sum = 0;

// Use final variable as loop limit - GOOD USE CASE
for (int i = 0; i < LIMIT; i++) {
    sum = sum + i;
}
print(sum);  // Should print 10 (0+1+2+3+4)

// Final variable INSIDE loop body - GOOD USE CASE
int product = 1;
for (int k = 1; k <= 3; k++) {
    final int MULTIPLIER = 2;  // Constant within each iteration
    product = product * MULTIPLIER;
}
print(product);  // Should print 8 (1*2*2*2)

// Using final as loop boundary
final int START = 10;
final int END = 15;
for (int j = START; j < END; j++) {
    print(j);  // Should print 10, 11, 12, 13, 14
}
print("Test passed"); // Test completed