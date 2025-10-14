// Test file for O1/O2 optimization - Dead Code Elimination in Switch/Case
// Tests unreachable code after return/break statements in switch cases

function testSwitchReturn(int x): string {
    switch (x) {
        case 1:
            return "one";
            print("Dead after return in case 1");  // Should be removed
            break;                                  // Should be removed
        case 2:
            return "two";
            print("Dead after return in case 2");  // Should be removed
            break;                                  // Should be removed
        default:
            return "other";
            print("Dead after return in default"); // Should be removed
            break;                                  // Should be removed
    }

    print("Dead after switch with all returns");  // Should be removed
    return "unreachable";                          // Should be removed
}

function testSwitchBreak(int x): int {
    int result = 0;

    switch (x) {
        case 1:
            result = 100;
            break;
            print("Dead after break in case 1");   // Should be removed
            result = 999;                           // Should be removed
        case 2:
            result = 200;
            break;
            print("Dead after break in case 2");   // Should be removed
            result = 888;                           // Should be removed
        default:
            result = 300;
            break;
            print("Dead after break in default");  // Should be removed
            result = 777;                           // Should be removed
    }

    // This is reachable, should NOT be removed
    return result;
}

function testSwitchMixed(int x): int {
    switch (x) {
        case 1:
            return 1;
            print("Dead after return");            // Should be removed
            break;                                  // Should be removed
        case 2:
            print("Reachable in case 2");
            break;
            print("Dead after break");             // Should be removed
        default:
            print("Reachable in default");
            // Fall through - no break or return
    }

    // This is reachable from case 2 and default
    print("Reachable after switch");
    return 0;
}

function testNestedSwitch(int x,int y): int {
    switch (x) {
        case 1:
            switch (y) {
                case 10:
                    return 110;
                    print("Dead in nested case");  // Should be removed
                case 20:
                    return 120;
                    print("Dead in nested case");  // Should be removed
            }
            return 100;
            print("Dead after nested switch");     // Should be removed
        case 2:
            return 200;
            print("Dead after return");            // Should be removed
    }

    return 0;
}

// Entry point
string result1 = testSwitchReturn(1);
print("Result 1: " + result1);

int result2 = testSwitchBreak(2);
print("Result 2: " + result2);

int result3 = testSwitchMixed(2);
print("Result 3: " + result3);

int result4 = testNestedSwitch(1, 10);
print("Result 4: " + result4);

print("Switch/Case Dead Code Test Complete!");
