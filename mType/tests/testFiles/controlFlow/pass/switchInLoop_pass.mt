// Switch statement nested in various loop constructs

// Test 1: Switch in for loop
print("Test 1: Switch in for loop");
for (int i = 0; i < 5; i = i + 1) {
    switch (i) {
        case 0:
            print("Zero");
            break;
        case 1:
            print("One");
            break;
        case 2:
            print("Two");
            break;
        case 3:
            print("Three");
            break;
        case 4:
            print("Four");
            break;
        default:
            print("Other");
            break;
    }
}

// Test 2: Switch in while loop
print("Test 2: Switch in while loop");
int counter = 0;
while (counter < 3) {
    switch (counter) {
        case 0:
            print("Counter is 0");
            break;
        case 1:
            print("Counter is 1");
            break;
        case 2:
            print("Counter is 2");
            break;
        default:
            print("Counter is other");
            break;
    }
    counter = counter + 1;
}

// Test 3: Nested loops with switch
print("Test 3: Nested loops with switch");
for (int i = 0; i < 2; i = i + 1) {
    for (int j = 0; j < 2; j = j + 1) {
        int sum = i + j;
        switch (sum) {
            case 0:
                print("Sum is 0");
                break;
            case 1:
                print("Sum is 1");
                break;
            case 2:
                print("Sum is 2");
                break;
            default:
                print("Sum is other");
                break;
        }
    }
}

// Test 4: Switch with break in loop (break only exits switch)
print("Test 4: Switch with break in loop");
for (int i = 0; i < 3; i = i + 1) {
    switch (i) {
        case 0:
            print("Case 0");
            break;  // Only breaks switch, not the loop
        case 1:
            print("Case 1");
            break;
        case 2:
            print("Case 2");
            break;
    }
    print("After switch in loop");
}

// Test 5: Loop with continue inside switch
print("Test 5: Loop with continue inside switch");
for (int i = 0; i < 5; i = i + 1) {
    if (i == 2) {
        continue;  // Skip when i is 2
    }
    switch (i) {
        case 0:
            print("Processing 0");
            break;
        case 1:
            print("Processing 1");
            break;
        case 3:
            print("Processing 3");
            break;
        case 4:
            print("Processing 4");
            break;
        default:
            print("Processing other");
            break;
    }
}

// Test 6: Switch controlling loop variable
print("Test 6: Switch controlling loop variable");
int loopVar = 0;
while (loopVar < 10) {
    switch (loopVar) {
        case 0:
            print("Start");
            loopVar = loopVar + 1;
            break;
        case 1:
            print("One");
            loopVar = loopVar + 2;  // Jump to 3
            break;
        case 3:
            print("Three");
            loopVar = loopVar + 1;
            break;
        case 4:
            print("Four");
            loopVar = loopVar + 10;  // Exit loop
            break;
        default:
            print("Other");
            loopVar = loopVar + 1;
            break;
    }
}

// Test 7: Multiple switches in same loop
print("Test 7: Multiple switches in same loop");
for (int i = 0; i < 2; i = i + 1) {
    switch (i) {
        case 0:
            print("First switch: 0");
            break;
        case 1:
            print("First switch: 1");
            break;
    }

    switch (i * 2) {
        case 0:
            print("Second switch: 0");
            break;
        case 2:
            print("Second switch: 2");
            break;
    }
}

// Test 8: Switch with fall-through in loop
print("Test 8: Switch with fall-through in loop");
for (int i = 0; i < 3; i = i + 1) {
    switch (i) {
        case 0:
            print("Falling from 0");
        case 1:
            print("At 1 or fell from 0");
            break;
        case 2:
            print("At 2");
            break;
    }
}

// Test 9: String switch in loop
print("Test 9: String switch in loop");
for (int i = 0; i < 3; i = i + 1) {
    string color = "";
    if (i == 0) {
        color = "red";
    } else if (i == 1) {
        color = "green";
    } else {
        color = "blue";
    }

    switch (color) {
        case "red":
            print("Color: Red");
            break;
        case "green":
            print("Color: Green");
            break;
        case "blue":
            print("Color: Blue");
            break;
        default:
            print("Color: Unknown");
            break;
    }
}

// Test 10: Complex nested structure
print("Test 10: Complex nested structure");
for (int outer = 0; outer < 2; outer = outer + 1) {
    switch (outer) {
        case 0:
            for (int inner = 0; inner < 2; inner = inner + 1) {
                switch (inner) {
                    case 0:
                        print("Outer 0, Inner 0");
                        break;
                    case 1:
                        print("Outer 0, Inner 1");
                        break;
                }
            }
            break;
        case 1:
            print("Outer 1");
            break;
    }
}

print("All switch-in-loop tests completed");
