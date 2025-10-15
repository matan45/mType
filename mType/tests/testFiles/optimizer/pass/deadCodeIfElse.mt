// Test file for O1/O2 optimization - Dead Code Elimination in If/Else
// Tests unreachable code after return statements in if/else branches

function testIfReturn(int x): int {
    if (x > 0) {
        return x * 2;
        print("Dead code in if branch");  // Should be removed
        int y = 100;                       // Should be removed
    } else {
        return x * -2;
        print("Dead code in else branch"); // Should be removed
        int z = 200;                       // Should be removed
    }

    print("Unreachable after if-else");   // Should be removed
    return 999;                            // Should be removed
}

function testNestedIf(int x): int {
    if (x > 10) {
        if (x > 20) {
            return x * 3;
            print("Dead in nested if");    // Should be removed
        } else {
            return x * 2;
            print("Dead in nested else");  // Should be removed
        }
        print("Dead after nested if-else"); // Should be removed
    }
    return x;
}

function testIfWithoutElse(int x): int {
    if (x > 0) {
        return x * 2;
        print("Dead after return in if");  // Should be removed
    }

    // This is reachable if x <= 0, so should NOT be removed
    print("Reachable if x <= 0");
    return x * -1;
}

function testMultipleReturns(int x): int {
    if (x > 0) {
        return 1;
    } else if (x < 0) {
        return -1;
    } else {
        return 0;
    }

    print("Dead after complete if-else chain"); // Should be removed
    return 999;                                  // Should be removed
}

// Entry point
int result1 = testIfReturn(5);
print("Result 1: " + result1);

int result2 = testNestedIf(25);
print("Result 2: " + result2);

int result3 = testIfWithoutElse(-5);
print("Result 3: " + result3);

int result4 = testMultipleReturns(0);
print("Result 4: " + result4);

print("If/Else Dead Code Test Complete!");
