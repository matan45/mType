// Switch statements with complex expressions and computations

// Test 1: Switch on arithmetic expressions
int a = 10;
int b = 5;
switch (a + b) {
    case 14:
        print("a + b = 14");
        break;
    case 15:
        print("a + b = 15");  // Should match (10 + 5 = 15)
        break;
    case 16:
        print("a + b = 16");
        break;
    default:
        print("Unexpected sum");
        break;
}

// Test 2: Switch on multiplication/division
int x = 12;
int y = 3;
switch (x / y) {
    case 3:
        print("x / y = 3");
        break;
    case 4:
        print("x / y = 4");  // Should match (12 / 3 = 4)
        break;
    case 5:
        print("x / y = 5");
        break;
    default:
        print("Unexpected division result");
        break;
}

// Test 3: Switch on modulo operation
int mod_test = 17;
switch (mod_test % 5) {
    case 0:
        print("Divisible by 5");
        break;
    case 1:
        print("Remainder 1");
        break;
    case 2:
        print("Remainder 2");  // Should match (17 % 5 = 2)
        break;
    case 3:
        print("Remainder 3");
        break;
    case 4:
        print("Remainder 4");
        break;
    default:
        print("Unexpected remainder");
        break;
}

// Test 4: Switch on function return value
function getTestValue(): int {
    return 42;
}

switch (getTestValue()) {
    case 41:
        print("Function returned 41");
        break;
    case 42:
        print("Function returned 42");  // Should match
        break;
    case 43:
        print("Function returned 43");
        break;
    default:
        print("Function returned unexpected value");
        break;
}

// Test 5: Switch on complex nested expressions
int base = 5;
switch ((base * 2) + (base - 1)) {
    case 13:
        print("Complex expression = 13");
        break;
    case 14:
        print("Complex expression = 14");  // Should match ((5*2) + (5-1) = 10+4 = 14)
        break;
    case 15:
        print("Complex expression = 15");
        break;
    default:
        print("Unexpected complex result");
        break;
}

// Test 6: Switch on boolean expressions converted to int
bool condition1 = true;
bool condition2 = false;

// Assuming true = 1, false = 0 in integer context
switch (condition1) {
    case 0:
        print("condition1 is false");
        break;
    case 1:
        print("condition1 is true");  // Should match
        break;
    default:
        print("Unexpected boolean value");
        break;
}

// Test 7: Switch with variable assignment in expression
int assignment_test = 0;
switch (assignment_test = 7) {  // Assignment returns the assigned value
    case 6:
        print("Assignment result = 6");
        break;
    case 7:
        print("Assignment result = 7");  // Should match
        break;
    case 8:
        print("Assignment result = 8");
        break;
    default:
        print("Unexpected assignment result");
        break;
}
print("assignment_test is now: " + assignment_test);  // Should be 7

// Test 8: Switch on ternary operator result
int ternary_input = 10;
switch (ternary_input > 5 ? 1 : 0) {
    case 0:
        print("Ternary result: false case");
        break;
    case 1:
        print("Ternary result: true case");  // Should match (10 > 5 is true, so 1)
        break;
    default:
        print("Unexpected ternary result");
        break;
}

// Test 9: Switch on string length (if strings have length property)
// This might not work if string length isn't available as integer
string test_string = "hello";
// Assuming we can get string length somehow - this might need to be adapted
// For now, let's use a computed value based on known string length
int string_length_sim = 5;  // Simulating length of "hello"
switch (string_length_sim) {
    case 4:
        print("String length = 4");
        break;
    case 5:
        print("String length = 5");  // Should match
        break;
    case 6:
        print("String length = 6");
        break;
    default:
        print("Unexpected string length");
        break;
}

// Test 10: Switch on chained function calls
function getValue1(): int {
    return 3;
}

function getValue2(int input): int {
    return input * 4;
}

switch (getValue2(getValue1())) {
    case 10:
        print("Chained result = 10");
        break;
    case 12:
        print("Chained result = 12");  // Should match (getValue2(3) = 3*4 = 12)
        break;
    case 14:
        print("Chained result = 14");
        break;
    default:
        print("Unexpected chained result");
        break;
}

// Test 11: Switch with maximum/minimum operations
int val1 = 15;
int val2 = 23;
int val3 = 8;

// Simulate max function
int max_val = val1;
if (val2 > max_val) {
    max_val = val2;
}
if (val3 > max_val) {
    max_val = val3;
}

switch (max_val) {
    case 15:
        print("Maximum value is 15");
        break;
    case 23:
        print("Maximum value is 23");  // Should match
        break;
    case 8:
        print("Maximum value is 8");
        break;
    default:
        print("Unexpected maximum value");
        break;
}

// Test 12: Switch on array-like access simulation
// Since we might not have arrays, simulate with multiple variables
int arr_0 = 100;
int arr_1 = 200;
int arr_2 = 300;
int index = 1;

int selected_value = 0;
switch (index) {
    case 0:
        selected_value = arr_0;
        break;
    case 1:
        selected_value = arr_1;  // Should match
        break;
    case 2:
        selected_value = arr_2;
        break;
    default:
        selected_value = -1;
        break;
}

switch (selected_value) {
    case 100:
        print("Selected array[0] = 100");
        break;
    case 200:
        print("Selected array[1] = 200");  // Should match
        break;
    case 300:
        print("Selected array[2] = 300");
        break;
    default:
        print("Unexpected selected value");
        break;
}

print("All complex expression switch tests completed");