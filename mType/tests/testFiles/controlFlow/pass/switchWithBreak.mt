// Switch statement with break statements - proper control flow

// Test 1: Basic switch with breaks
int test1 = 2;
switch (test1) {
    case 1:
        print("Case 1");
        break;
    case 2:
        print("Case 2");  // Should match and stop here
        break;
    case 3:
        print("Case 3");
        break;
    default:
        print("Default");
        break;
}

// Test 2: Switch without breaks (fall-through behavior)
int test2 = 2;
switch (test2) {
    case 1:
        print("Falling from case 1");
    case 2:
        print("Matched case 2, continuing");  // Should match and continue
    case 3:
        print("Fell through to case 3");
    default:
        print("Fell through to default");
}

// Test 3: Mixed break and fall-through
int test3 = 1;
switch (test3) {
    case 1:
        print("Case 1 - will fall through");  // Matches here
    case 2:
        print("Fell through from case 1");
        break;  // Break here to prevent further fall-through
    case 3:
        print("This should not print");
    default:
        print("This should not print either");
}

// Test 4: Break in nested constructs within switch
int test4 = 3;
switch (test4) {
    case 1:
        print("Case 1");
        for (int i = 0; i < 3; i = i + 1) {
            if (i == 1) {
                break;  // This breaks the loop, not the switch
            }
            print(i);
        }
        print("After loop in case 1");
        break;
    case 3:
        print("Case 3");  // Should match
        for (int j = 0; j < 2; j = j + 1) {
            print(j + 100);
        }
        break;
    default:
        print("Default");
}

// Test 5: Multiple statements before break
int test5 = 4;
switch (test5) {
    case 4:
        print("First statement in case 4");  // Should match
        int temp = 42;
        print(temp);
        print("About to break from case 4");
        break;
    case 5:
        print("This should not execute");
    default:
        print("This should not execute either");
}

// Test 6: Break in default case
int test6 = 999;
switch (test6) {
    case 1:
        print("Case 1");
        break;
    case 2:
        print("Case 2");
        break;
    default:
        print("Default case");  // Should match
        print("Second statement in default");
        break;  // Break in default (good practice)
}

// Test 7: No break in last case/default
int test7 = 10;
switch (test7) {
    case 5:
        print("Case 5");
        break;
    case 10:
        print("Case 10 - no break");  // Should match
        // No break - but it's the last case so it doesn't matter
}

// Test 8: Complex control flow with breaks
int test8 = 2;
bool shouldContinue = true;
switch (test8) {
    case 1:
        print("Case 1");
        if (shouldContinue) {
            print("Continuing in case 1");
        }
        break;
    case 2:
        print("Case 2");  // Should match
        shouldContinue = false;
        if (shouldContinue) {
            print("This should not print");
        } else {
            print("ShouldContinue is false");
        }
        break;
    default:
        print("Default case");
        break;
}

// Test 9: Switch inside loop with break
for (int i = 1; i <= 3; i = i + 1) {
    switch (i) {
        case 1:
            print("Loop iteration 1");
            break;
        case 2:
            print("Loop iteration 2");
            break; // This breaks the switch, not the loop
        case 3:
            print("Loop iteration 3");
            break;
        default:
            print("Unexpected loop value");
            break;
    }
}

print("All break statement tests completed");