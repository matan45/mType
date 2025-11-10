// Test deeply nested loops with multiple exit points

// Triple nested loops with conditional breaks
for (int i = 1; i <= 2; i++) {
    for (int j = 1; j <= 2; j++) {
        for (int k = 1; k <= 3; k++) {
            if (k == 2 && j == 2) {
                break; // Break innermost
            }
            print(i * 100 + j * 10 + k);
        }
    }
}

// Nested loops with multiple continue conditions
int outer = 0;
while (outer < 3) {
    outer = outer + 1;
    int middle = 0;
    while (middle < 3) {
        middle = middle + 1;
        if (middle == 2 && outer == 2) {
            continue; // Skip this iteration
        }
        int inner = 0;
        while (inner < 2) {
            inner = inner + 1;
            if (inner == 2 && middle == 3) {
                continue;
            }
            print(outer * 100 + middle * 10 + inner);
        }
    }
}

// Complex nested structure with early exits
bool exitAll = false;
for (int x = 1; x <= 3; x++) {
    if (exitAll) break;
    for (int y = 1; y <= 3; y++) {
        if (exitAll) break;
        for (int z = 1; z <= 3; z++) {
            if (x + y + z > 6) {
                exitAll = true;
                print("Exit triggered");
                break;
            }
            print(x * 100 + y * 10 + z);
        }
    }
}

print("Test passed");
