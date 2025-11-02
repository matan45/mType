// Switch statement with intentional fall-through behavior

// Test 1: Simple fall-through
int value1 = 1;
switch (value1) {
    case 1:
        print("Case 1");
        // Intentionally no break - falls through to case 2
    case 2:
        print("Case 2 (fell through from 1)");
        break;
    case 3:
        print("Case 3");
        break;
    default:
        print("Default");
        break;
}

// Test 2: Multiple fall-throughs
int value2 = 5;
switch (value2) {
    case 5:
        print("Starting at 5");
        // Falls through
    case 6:
        print("Fell to 6");
        // Falls through
    case 7:
        print("Fell to 7");
        // Falls through
    case 8:
        print("Fell to 8");
        break;
    default:
        print("Default");
        break;
}

// Test 3: Fall-through to default
int value3 = 10;
switch (value3) {
    case 10:
        print("Case 10");
        // Falls through to default
    default:
        print("Fell through to default");
        break;
}

// Test 4: Selective fall-through
int value4 = 2;
switch (value4) {
    case 1:
        print("Case 1");
        break;  // No fall-through here
    case 2:
        print("Case 2");
        // Falls through
    case 3:
        print("Case 3 (fell from 2)");
        break;  // Stop here
    case 4:
        print("Case 4");
        break;
    default:
        print("Default");
        break;
}

// Test 5: All cases fall through
int value5 = 1;
switch (value5) {
    case 1:
        print("Step 1");
    case 2:
        print("Step 2");
    case 3:
        print("Step 3");
    default:
        print("Final step");
}

// Test 6: Fall-through with multiple statements
int value6 = 7;
switch (value6) {
    case 7:
        print("First statement in case 7");
        print("Second statement in case 7");
        // Falls through
    case 8:
        print("First statement in case 8");
        break;
    default:
        print("Default");
        break;
}

// Test 7: Fall-through pattern for range simulation
int dayOfWeek = 1;  // Monday
switch (dayOfWeek) {
    case 1:  // Monday
    case 2:  // Tuesday
    case 3:  // Wednesday
    case 4:  // Thursday
    case 5:  // Friday
        print("Weekday");
        break;
    case 6:  // Saturday
    case 7:  // Sunday
        print("Weekend");
        break;
    default:
        print("Invalid day");
        break;
}

// Test 8: Complex fall-through with variable modifications
int counter = 0;
int testVal = 2;
switch (testVal) {
    case 1:
        counter = counter + 1;
    case 2:
        counter = counter + 10;  // This executes
    case 3:
        counter = counter + 100;  // This also executes due to fall-through
        break;
    case 4:
        counter = counter + 1000;
        break;
    default:
        counter = counter + 10000;
        break;
}
print(counter);  // Should be 110

print("All fall-through tests completed");
