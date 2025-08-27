// Nested loops with break
for (int i = 1; i <= 3; i++) {
    print(i * 100); // Outer loop indicator
    for (int j = 1; j <= 5; j++) {
        if (j == 3) {
            break; // Only breaks inner loop
        }
        print(i * 10 + j);
    }
}

// Nested loops with continue
for (int x = 1; x <= 2; x++) {
    for (int y = 1; y <= 4; y++) {
        if (y == 2) {
            continue; // Skip y=2
        }
        print(x * 100 + y);
    }
}
print("Test passed"); // Test completed