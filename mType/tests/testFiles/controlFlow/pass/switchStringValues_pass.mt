// Switch statement with string values - testing string matching

// Test 1: Basic string switch
string color = "red";
switch (color) {
    case "red":
        print("Color is red");
        break;
    case "blue":
        print("Color is blue");
        break;
    case "green":
        print("Color is green");
        break;
    default:
        print("Unknown color");
        break;
}

// Test 2: Empty string case
string empty = "";
switch (empty) {
    case "":
        print("Empty string matched");
        break;
    case "nonempty":
        print("Should not print");
        break;
    default:
        print("Default case");
        break;
}

// Test 3: String with spaces and special characters
string message = "Hello World!";
switch (message) {
    case "Hello":
        print("Just Hello");
        break;
    case "Hello World!":
        print("Full greeting matched");
        break;
    case "Goodbye":
        print("Farewell");
        break;
    default:
        print("Unknown message");
        break;
}

// Test 4: Case sensitivity test
string word = "TEST";
switch (word) {
    case "test":
        print("Lowercase");
        break;
    case "TEST":
        print("Uppercase matched");
        break;
    case "Test":
        print("Title case");
        break;
    default:
        print("No match");
        break;
}

// Test 5: String switch with default
string unknown = "xyz123";
switch (unknown) {
    case "abc":
        print("abc");
        break;
    case "def":
        print("def");
        break;
    default:
        print("Default for unknown string");
        break;
}

// Test 6: Multi-word strings
string phrase = "quick brown fox";
switch (phrase) {
    case "lazy dog":
        print("Lazy dog");
        break;
    case "quick brown fox":
        print("Quick brown fox matched");
        break;
    case "jumps over":
        print("Jumps over");
        break;
    default:
        print("No phrase match");
        break;
}

// Test 7: String concatenation result in switch
string part1 = "Hello";
string part2 = "World";
string combined = part1 + part2;
switch (combined) {
    case "HelloWorld":
        print("Concatenated string matched");
        break;
    case "Hello World":
        print("With space");
        break;
    default:
        print("No match for combined");
        break;
}

print("All string switch tests completed");
