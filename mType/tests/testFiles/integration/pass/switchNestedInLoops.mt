// Switch statements nested inside various loop constructs

// Test 1: Switch inside for loop
print("Test 1: Switch inside for loop");
for (int i = 1; i <= 4; i = i + 1) {
    switch (i) {
        case 1:
            print("For loop iteration 1");
            break;
        case 2:
            print("For loop iteration 2");
            break;
        case 3:
            print("For loop iteration 3");
            break;
        default:
            print("For loop default case");
            break;
    }
}

// Test 2: Switch inside while loop
print("Test 2: Switch inside while loop");
int count = 1;
while (count <= 3) {
    switch (count) {
        case 1:
            print("While loop count 1");
            break;
        case 2:
            print("While loop count 2");
            break;
        case 3:
            print("While loop count 3");
            break;
        default:
            print("While loop unexpected count");
            break;
    }
    count = count + 1;
}

// Test 3: Nested loops with switch
print("Test 3: Nested loops with switch");
for (int outer = 1; outer <= 2; outer = outer + 1) {
    print("Outer loop:");
    for (int inner = 1; inner <= 3; inner = inner + 1) {
        switch (inner) {
            case 1:
                print("  Inner case 1");
                break;
            case 2:
                print("  Inner case 2");
                break;
            case 3:
                print("  Inner case 3");
                break;
            default:
                print("  Inner default");
                break;
        }
    }
}

// Test 4: Switch with break and continue interactions
print("Test 4: Switch with break affecting loop");
for (int i = 1; i <= 5; i = i + 1) {
    switch (i) {
        case 1:
            print("Case 1 - continuing loop");
            break; // Break switch, continue loop
        case 2:
            print("Case 2 - continuing loop");  
            break;
        case 3:
            print("Case 3 - would break loop");
            // In some languages you might use labeled break to break the loop
            break;
        case 4:
            print("Case 4 - continuing loop");
            break;
        default:
            print("Default case");
            break;
    }
    print("End of loop iteration");
}

// Test 5: Switch inside do-while loop (if supported)
print("Test 5: Switch inside do-while style loop");
int counter = 0;
while (true) {
    counter = counter + 1;
    switch (counter) {
        case 1:
            print("Do-while iteration 1");
            break;
        case 2:
            print("Do-while iteration 2");
            break;
        case 3:
            print("Do-while iteration 3 - breaking loop");
            // We'll use a flag to break the outer loop
            counter = 10; // Force exit condition
            break;
        default:
            print("Do-while default");
            break;
    }
    if (counter >= 10) {
        break; // Break the while loop
    }
}

// Test 6: Multiple switches in same loop
print("Test 6: Multiple switches in same loop");
for (int i = 1; i <= 2; i = i + 1) {
    // First switch
    switch (i) {
        case 1:
            print("First switch - case 1");
            break;
        case 2:
            print("First switch - case 2");
            break;
        default:
            print("First switch - default");
            break;
    }
    
    // Second switch
    switch (i * 2) {
        case 2:
            print("Second switch - case 2");
            break;
        case 4:
            print("Second switch - case 4");
            break;
        default:
            print("Second switch - default");
            break;
    }
}

// Test 7: Switch controlling loop continuation
print("Test 7: Switch controlling loop flow");
int value = 1;
while (value <= 5) {
    bool shouldContinue = true;
    switch (value) {
        case 1:
            print("Processing value 1");
            break;
        case 2:
            print("Processing value 2");
            break;
        case 3:
            print("Skipping value 3");
            shouldContinue = false;
            break;
        case 4:
            print("Processing value 4");
            break;
        case 5:
            print("Final value 5");
            break;
        default:
            print("Unexpected value");
            break;
    }
    
    if (shouldContinue) {
        print("Processed value: " + value);
    } else {
        print("Skipped value: " + value);
    }
    
    value = value + 1;
}

// Test 8: Deeply nested structure
print("Test 8: Deeply nested structure");
for (int level1 = 1; level1 <= 2; level1 = level1 + 1) {
    switch (level1) {
        case 1:
            print("Level 1, Case 1");
            for (int level2 = 1; level2 <= 2; level2 = level2 + 1) {
                switch (level2) {
                    case 1:
                        print("  Level 2, Case 1");
                        break;
                    case 2:
                        print("  Level 2, Case 2");
                        break;
                    default:
                        print("  Level 2, Default");
                        break;
                }
            }
            break;
        case 2:
            print("Level 1, Case 2");
            break;
        default:
            print("Level 1, Default");
            break;
    }
}

print("All nested switch tests completed");