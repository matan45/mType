// Switch statements with edge case values and boundary conditions

// Test 1: Zero value
int test_zero = 0;
switch (test_zero) {
    case -1:
        print("Negative one");
        break;
    case 0:
        print("Zero value");  // Should match
        break;
    case 1:
        print("Positive one");
        break;
    default:
        print("Not zero, -1, or 1");
        break;
}

// Test 2: Very large positive numbers
int large_positive = 2147483647;  // Max 32-bit signed integer
switch (large_positive) {
    case 2147483646:
        print("Max int minus 1");
        break;
    case 2147483647:
        print("Maximum 32-bit signed integer");  // Should match
        break;
    default:
        print("Not max integer");
        break;
}

// Test 3: Very large negative numbers
int large_negative = -2147483647;  // Near min 32-bit signed integer
switch (large_negative) {
    case -2147483647:
        print("Near minimum 32-bit signed integer");  // Should match
        break;
    case -2147483646:
        print("Min int plus 2");
        break;
    default:
        print("Not min integer");
        break;
}

// Test 4: Powers of 2 (common edge cases)
int power_of_two = 1024;
switch (power_of_two) {
    case 512:
        print("2^9");
        break;
    case 1024:
        print("2^10");  // Should match
        break;
    case 2048:
        print("2^11");
        break;
    default:
        print("Not a tested power of 2");
        break;
}

// Test 5: Numbers around zero boundary
int near_zero = -1;
switch (near_zero) {
    case -2:
        print("Negative two");
        break;
    case -1:
        print("Negative one");  // Should match
        break;
    case 0:
        print("Zero");
        break;
    case 1:
        print("Positive one");
        break;
    default:
        print("Not near zero");
        break;
}

// Test 6: Single digit numbers (0-9)
for (int digit = 0; digit <= 9; digit = digit + 1) {
    switch (digit) {
        case 0: print("Digit zero"); break;
        case 1: print("Digit one"); break;
        case 2: print("Digit two"); break;
        case 3: print("Digit three"); break;
        case 4: print("Digit four"); break;
        case 5: print("Digit five"); break;
        case 6: print("Digit six"); break;
        case 7: print("Digit seven"); break;
        case 8: print("Digit eight"); break;
        case 9: print("Digit nine"); break;
        default: print("Not a single digit"); break;
    }
}

// Test 7: ASCII-like values (if integers represent characters)
int ascii_A = 65;
switch (ascii_A) {
    case 64:
        print("ASCII @");
        break;
    case 65:
        print("ASCII A");  // Should match
        break;
    case 66:
        print("ASCII B");
        break;
    default:
        print("Other ASCII value");
        break;
}

// Test 8: Fibonacci numbers
int fib_test = 13;
switch (fib_test) {
    case 1: print("Fibonacci 1"); break;
    case 2: print("Fibonacci 2"); break;
    case 3: print("Fibonacci 3"); break;
    case 5: print("Fibonacci 5"); break;
    case 8: print("Fibonacci 8"); break;
    case 13: print("Fibonacci 13"); break;  // Should match
    case 21: print("Fibonacci 21"); break;
    default: print("Not a tested Fibonacci number"); break;
}

// Test 9: Prime numbers
int prime_test = 17;
switch (prime_test) {
    case 2: print("Prime 2"); break;
    case 3: print("Prime 3"); break;
    case 5: print("Prime 5"); break;
    case 7: print("Prime 7"); break;
    case 11: print("Prime 11"); break;
    case 13: print("Prime 13"); break;
    case 17: print("Prime 17"); break;  // Should match
    case 19: print("Prime 19"); break;
    default: print("Not a tested prime number"); break;
}

// Test 10: Boundary values around common limits
int boundary_test = 255;
switch (boundary_test) {
    case 127:
        print("Max signed 8-bit");
        break;
    case 128:
        print("Min signed 8-bit (as positive)");
        break;
    case 255:
        print("Max unsigned 8-bit");  // Should match
        break;
    case 256:
        print("Overflow 8-bit");
        break;
    default:
        print("Other boundary value");
        break;
}

// Test 11: Multiples of common numbers
int multiple_test = 100;
switch (multiple_test) {
    case 50:   // 50 * 2
        print("Multiple: 50");
        break;
    case 75:   // 25 * 3
        print("Multiple: 75");
        break;
    case 100:  // 10 * 10 or 25 * 4
        print("Multiple: 100");  // Should match
        break;
    case 125:  // 25 * 5
        print("Multiple: 125");
        break;
    default:
        print("Not a tested multiple");
        break;
}

// Test 12: Consecutive negative numbers
int negative_seq = -3;
switch (negative_seq) {
    case -5:
        print("Negative five");
        break;
    case -4:
        print("Negative four");
        break;
    case -3:
        print("Negative three");  // Should match
        break;
    case -2:
        print("Negative two");
        break;
    case -1:
        print("Negative one");
        break;
    default:
        print("Other negative number");
        break;
}

// Test 13: Hexadecimal-like values (powers of 16)
int hex_test = 256;
switch (hex_test) {
    case 16:    // 0x10
        print("16 (0x10)");
        break;
    case 32:    // 0x20
        print("32 (0x20)");
        break;
    case 64:    // 0x40
        print("64 (0x40)");
        break;
    case 128:   // 0x80
        print("128 (0x80)");
        break;
    case 256:   // 0x100
        print("256 (0x100)");  // Should match
        break;
    default:
        print("Other hex-like value");
        break;
}

// Test 14: Random-looking but specific values
int random_test = 12345;
switch (random_test) {
    case 12344:
        print("Almost 12345");
        break;
    case 12345:
        print("Exactly 12345");  // Should match
        break;
    case 12346:
        print("Just over 12345");
        break;
    default:
        print("Not near 12345");
        break;
}

// Test 15: Testing integer overflow behavior (if applicable)
// This depends on how the language handles integer arithmetic
int overflow_test = 2147483647;  // Max int
overflow_test = overflow_test + 1;  // This might overflow to -2147483647 or wrap

switch (overflow_test) {
    case -2147483647:
        print("Integer overflowed near min value");
        break;
    case 2147483647:
        print("Integer stayed at max (if supported)");
        break;
    default:
        print("Overflow behavior: " + overflow_test);
        break;
}

print("All edge case value tests completed");