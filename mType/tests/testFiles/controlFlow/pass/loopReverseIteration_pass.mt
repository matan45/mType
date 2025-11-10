// Test loops with reverse iteration (counting down)

// For loop counting down
for (int i = 10; i > 0; i--) {
    print(i);
}
print("Countdown complete");

// While loop counting down
int count = 5;
while (count > 0) {
    print(count);
    count = count - 1;
}
print("While countdown complete");

// Do-while counting down
int x = 3;
do {
    print(x);
    x = x - 1;
} while (x > 0);
print("Do-while countdown complete");

// Reverse with step
for (int j = 20; j >= 10; j = j - 2) {
    print(j);
}
print("Step countdown complete");

// Count down to negative
int val = 2;
while (val > -3) {
    print(val);
    val = val - 1;
}
print("Negative countdown complete");

print("Test passed");
