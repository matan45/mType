// Switch statement without default case - should handle gracefully

// Test 1: Match a case (no default)
int value1 = 2;
switch (value1) {
    case 1:
        print("Case 1");
        break;
    case 2:
        print("Case 2 matched");
        break;
    case 3:
        print("Case 3");
        break;
}

// Test 2: No match and no default (should continue silently)
int value2 = 999;
switch (value2) {
    case 1:
        print("Case 1");
        break;
    case 2:
        print("Case 2");
        break;
    case 3:
        print("Case 3");
        break;
}
print("Continued after no match");

// Test 3: String switch without default
string color = "red";
switch (color) {
    case "red":
        print("Red color");
        break;
    case "blue":
        print("Blue color");
        break;
    case "green":
        print("Green color");
        break;
}

// Test 4: No match in string switch without default
string unknown = "yellow";
switch (unknown) {
    case "red":
        print("Red");
        break;
    case "blue":
        print("Blue");
        break;
    case "green":
        print("Green");
        break;
}
print("No match for yellow");

// Test 5: Single case without default
int single = 42;
switch (single) {
    case 42:
        print("Answer to everything");
        break;
}

// Test 6: Single case no match without default
int noMatch = 43;
switch (noMatch) {
    case 42:
        print("This won't print");
        break;
}
print("After single case no match");

// Test 7: Multiple statements in matched case without default
int value7 = 5;
switch (value7) {
    case 5:
        print("First statement");
        int temp = 100;
        print(temp);
        print("Third statement");
        break;
    case 6:
        print("Case 6");
        break;
}

// Test 8: Fall-through without default
int value8 = 1;
switch (value8) {
    case 1:
        print("Case 1 falling");
    case 2:
        print("Case 2");
        break;
    case 3:
        print("Case 3");
        break;
}

// Test 9: No match with fall-through and no default
int value9 = 10;
switch (value9) {
    case 1:
        print("Case 1");
    case 2:
        print("Case 2");
        break;
    case 3:
        print("Case 3");
        break;
}
print("After fall-through no match");

print("All no-default tests completed");
