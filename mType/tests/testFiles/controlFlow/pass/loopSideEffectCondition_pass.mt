// Test loops with side effects in conditions and assignments

// Counter increment in condition
int counter = 0;
while ((counter = counter + 1) <= 5) {
    print(counter);
}
print("Final counter:");
print(counter);

// Multiple operations in for loop
int sum = 0;
for (int i = 1; i <= 3; i++) {
    sum = sum + i;
    print(i);
    print(sum);
}

// Complex condition with multiple checks
int x = 0;
int y = 10;
while (x < 5 && y > 5) {
    x = x + 1;
    y = y - 1;
    print(x);
    print(y);
}

// Assignment in condition expression
int value = 0;
int target = 3;
while ((value = value + 1) != target) {
    print(value);
}
print("Reached target:");
print(value);

print("Test passed");
