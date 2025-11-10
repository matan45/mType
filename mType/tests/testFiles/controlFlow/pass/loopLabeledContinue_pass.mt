// Test labeled continue for outer loop
// Using flag to simulate labeled continue behavior

// Skip to next outer iteration when condition met
for (int i = 1; i <= 3; i++) {
    bool skipOuter = false;
    for (int j = 1; j <= 3; j++) {
        if (i == 2 && j == 2) {
            skipOuter = true;
            break; // Exit inner to skip rest of outer iteration
        }
        print(i * 10 + j);
    }
    if (skipOuter) {
        continue; // Skip rest of outer iteration
    }
    print(i * 100); // Should not print for i=2
}

// Another pattern: conditional outer loop continuation
int outerCount = 0;
while (outerCount < 4) {
    outerCount = outerCount + 1;
    bool skipRest = false;

    int innerCount = 0;
    while (innerCount < 3) {
        innerCount = innerCount + 1;
        if (outerCount == 2 && innerCount == 1) {
            skipRest = true;
            break;
        }
        print(outerCount * 10 + innerCount);
    }

    if (!skipRest) {
        print(outerCount * 100);
    }
}

print("Test passed");
