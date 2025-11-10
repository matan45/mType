// Test labeled break to exit outer loop from inner
// Note: If mType doesn't support labeled breaks, this tests deep nesting with flags

// Using flag variable to simulate labeled break
bool exitOuter = false;
for (int i = 1; i <= 3; i++) {
    print(i * 100); // Outer loop marker
    for (int j = 1; j <= 5; j++) {
        print(i * 10 + j);
        if (i == 2 && j == 3) {
            exitOuter = true;
            break; // Break inner
        }
    }
    if (exitOuter) {
        break; // Break outer
    }
}
print("Exited both loops");

// Another test with nested loops and conditional outer break
int found = 0;
for (int x = 1; x <= 4; x++) {
    for (int y = 1; y <= 4; y++) {
        if (x * y == 6) {
            found = x * 100 + y;
            break;
        }
    }
    if (found > 0) {
        break;
    }
}
print(found);

print("Test passed");
