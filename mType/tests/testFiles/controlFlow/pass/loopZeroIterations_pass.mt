// Test loops with zero iterations - condition false from start

// While loop with false condition
int count = 0;
while (false) {
    count = count + 1;
    print("Should not execute");
}
print(count); // Should print 0

// For loop with false condition
for (int i = 10; i < 5; i++) {
    print("Should not execute");
}
print("For loop skipped");

// While loop with condition that starts false
int x = 100;
while (x < 50) {
    print("Should not execute");
    x = x + 1;
}
print(x); // Should print 100

// Do-while loop always executes at least once
int y = 0;
do {
    y = y + 1;
} while (false);
print(y); // Should print 1

print("Test passed");
