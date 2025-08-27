// Test break in while loop
int count = 0;
while (true) {
    count = count + 1;
    print(count);
    if (count == 3) {
        break;
    }
}
print("Test passed"); // Test completed

// Test break in for loop
for (int i = 0; i < 10; i++) {
    if (i == 5) {
        break;
    }
    print(i);
}
print("Test passed"); // Test completed

// Test break in do-while loop
int x = 0;
do {
    x = x + 1;
    if (x == 2) {
        break;
    }
    print(x);
} while (x < 10);

print("Test passed"); // Test completed