// Switch statement with only default case - edge case testing

// Test 1: Only default case with int
int value1 = 42;
switch (value1) {
    default:
        print("Only default case - int");
        break;
}

// Test 2: Only default case with string
string text = "hello";
switch (text) {
    default:
        print("Only default case - string");
        break;
}

// Test 3: Only default case with variable
int dynamic = 0;
switch (dynamic) {
    default:
        print("Only default - zero value");
        break;
}

// Test 4: Only default case with negative number
int negative = -100;
switch (negative) {
    default:
        print("Only default - negative");
        break;
}

// Test 5: Multiple statements in only-default switch
int value5 = 999;
switch (value5) {
    default:
        print("First statement in default");
        int temp = value5 * 2;
        print(temp);
        print("Third statement in default");
        break;
}

// Test 6: Only default without break
int value6 = 1;
switch (value6) {
    default:
        print("Default without break");
}

// Test 7: Only default in nested context
for (int i = 0; i < 2; i = i + 1) {
    switch (i) {
        default:
            print("Default in loop iteration");
            break;
    }
}

// Test 8: Only default with fall-through to nothing
int value8 = 123;
switch (value8) {
    default:
        print("Falling from default");
        // No break, but nothing to fall through to
}

// Test 9: Only default modifying variables
int counter = 0;
switch (counter) {
    default:
        counter = counter + 100;
        break;
}
print(counter);

// Test 10: Empty string with only default
string empty = "";
switch (empty) {
    default:
        print("Default for empty string");
        break;
}

print("All only-default tests completed");
