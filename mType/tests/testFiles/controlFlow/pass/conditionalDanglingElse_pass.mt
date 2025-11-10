// Test nested if-else matching (dangling else problem)

// Test 1: Else should match inner if
if (true) {
    if (false) {
        print(100);
    } else {
        print(200);
    }
}

// Test 2: Else should match outer if
if (false) {
    if (true) {
        print(300);
    }
} else {
    print(400);
}

// Test 3: Multiple nested levels
if (true) {
    if (true) {
        if (false) {
            print(500);
        } else {
            print(600);
        }
    } else {
        print(700);
    }
} else {
    print(800);
}

// Test 4: Complex nesting with multiple elses
if (true) {
    if (false) {
        print(900);
    } else {
        if (true) {
            print(1000);
        } else {
            print(1100);
        }
    }
} else {
    print(1200);
}

// Test 5: Deeply nested without braces ambiguity
if (true) {
    if (true) {
        if (true) {
            print(1300);
        }
    }
}

// Test 6: Else-if chain with nested if
if (false) {
    print(1400);
} else if (true) {
    if (true) {
        print(1500);
    } else {
        print(1600);
    }
} else {
    print(1700);
}

print("Test passed"); // Test completed
