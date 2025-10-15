// Test file for O1/O2 optimization - Dead Code Elimination in Loops
// Tests unreachable code after return/break/continue statements in loops

function testWhileReturn(int x): int {
    while (x > 0) {
        if (x > 10) {
            return x * 2;
            print("Dead after return in while");   // Should be removed
            x = x - 1;                              // Should be removed
        }
        x = x - 1;
    }
    return x;
}

function testWhileBreak(int x): int {
    int sum = 0;
    while (x > 0) {
        sum = sum + x;
        if (sum > 50) {
            break;
            print("Dead after break in while");    // Should be removed
            sum = 999;                              // Should be removed
        }
        x = x - 1;
    }
    return sum;
}

function testWhileContinue(int x): int {
    int sum = 0;
    while (x > 0) {
        if (x % 2 == 0) {
            x = x - 1;
            continue;
            print("Dead after continue");          // Should be removed
            sum = sum + 1;                          // Should be removed
        }
        sum = sum + x;
        x = x - 1;
    }
    return sum;
}

function testForReturn(int n): int {
    for (int i = 0; i < n; i = i + 1) {
        if (i == 5) {
            return i * 10;
            print("Dead after return in for");     // Should be removed
            i = i + 1;                              // Should be removed
        }
    }
    return -1;
}

function testForBreak(int n): int {
    int sum = 0;
    for (int i = 0; i < n; i = i + 1) {
        sum = sum + i;
        if (sum > 20) {
            break;
            print("Dead after break in for");      // Should be removed
            sum = 999;                              // Should be removed
        }
    }
    return sum;
}

function testForContinue(int n): int {
    int sum = 0;
    for (int i = 0; i < n; i = i + 1) {
        if (i % 2 == 0) {
            continue;
            print("Dead after continue in for");   // Should be removed
            sum = sum + 1;                          // Should be removed
        }
        sum = sum + i;
    }
    return sum;
}

function testNestedLoops(int n): int {
    for (int i = 0; i < n; i = i + 1) {
        for (int j = 0; j < n; j = j + 1) {
            if (i * j > 20) {
                return i * j;
                print("Dead after return in nested loop"); // Should be removed
            }
        }
        print("Reachable after inner loop");
    }
    return 0;
}

function testLoopWithAllReturns(int x): int {
    while (x > 0) {
        if (x > 10) {
            return 1;
        } else {
            return -1;
        }
        print("Dead after complete if-else");      // Should be removed
        x = x - 1;                                  // Should be removed
    }

    print("Dead after loop with all returns");     // Should be removed
    return 999;                                     // Should be removed
}

function testDoWhileReturn(int x): int {
    do {
        if (x > 5) {
            return x * 2;
            print("Dead after return in do-while"); // Should be removed
            x = x - 1;                               // Should be removed
        }
        x = x - 1;
    } while (x > 0);

    return x;
}

function testDoWhileBreak(int x): int {
    int sum = 0;
    do {
        sum = sum + x;
        if (sum > 30) {
            break;
            print("Dead after break in do-while"); // Should be removed
            sum = 999;                              // Should be removed
        }
        x = x - 1;
    } while (x > 0);

    return sum;
}

// Entry point
int result1 = testWhileReturn(15);
print("While return: " + result1);

int result2 = testWhileBreak(20);
print("While break: " + result2);

int result3 = testWhileContinue(10);
print("While continue: " + result3);

int result4 = testForReturn(10);
print("For return: " + result4);

int result5 = testForBreak(10);
print("For break: " + result5);

int result6 = testForContinue(10);
print("For continue: " + result6);

int result7 = testNestedLoops(5);
print("Nested loops: " + result7);

int result8 = testLoopWithAllReturns(15);
print("Loop all returns: " + result8);

int result9 = testDoWhileReturn(10);
print("Do-while return: " + result9);

int result10 = testDoWhileBreak(10);
print("Do-while break: " + result10);

print("Loop Dead Code Test Complete!");
