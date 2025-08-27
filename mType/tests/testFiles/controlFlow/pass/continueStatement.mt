// Test continue in while loop - skip even numbers
int i = 0;
while (i < 6) {
    i = i + 1;
    if (i % 2 == 0) {
        continue;
    }
    print(i); // Should print 1, 3, 5
}

// Test continue in for loop - skip multiples of 3
for (int j = 1; j <= 10; j++) {
    if (j % 3 == 0) {
        continue;
    }
    print(j); // Should print 1, 2, 4, 5, 7, 8, 10
}

// Test continue in do-while loop
int k = 0;
do {
    k = k + 1;
    if (k == 2 || k == 4) {
        continue;
    }
    print(k); // Should print 1, 3, 5
} while (k < 5);
print("Test passed"); // Test completed