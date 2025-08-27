// Test all compound assignment operators
int a = 10;
print(a);  // 10

a += 5;
print(a);  // 15

a -= 3;
print(a);  // 12

a *= 2;
print(a);  // 24

a /= 4;
print(a);  // 6

a %= 4;
print(a);  // 2

// Test with float
float f = 10.5;
f += 2.5;
print(f);  // 13.0

f *= 2.0;
print(f);  // 26.0

// Test in loops
int sum = 0;
for (int i = 0; i < 5; i++) {
    sum += i;
}
print(sum);  // 10

// Test with expressions
int x = 10;
x += 5 * 2;  // x = x + (5 * 2)
print(x);    // 20

// Test chaining (if you want to support it)
int y = 5;
y += 3;
y *= 2;
print(y);    // 16
print("Test passed"); // Test completed