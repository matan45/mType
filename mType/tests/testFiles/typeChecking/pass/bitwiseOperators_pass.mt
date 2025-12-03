// Test bitwise operators: &, |, ^, <<, >>, ~



    function main(): void {
        // Test bitwise AND
        int a = 12;  // 1100 in binary
        int b = 10;  // 1010 in binary
        int andResult = a & b;  // Should be 8 (1000)
        print("AND: " + andResult);

        // Test bitwise OR
        int orResult = a | b;  // Should be 14 (1110)
        print("OR: " + orResult);

        // Test bitwise XOR
        int xorResult = a ^ b;  // Should be 6 (0110)
        print("XOR: " + xorResult);

        // Test left shift
        int c = 3;
        int leftShift = c << 2;  // Should be 12 (3 * 4)
        print("Left shift: " + leftShift);

        // Test right shift
        int d = 16;
        int rightShift = d >> 2;  // Should be 4 (16 / 4)
        print("Right shift: " + rightShift);

        // Test bitwise NOT
        int e = 0;
        int notResult = ~e;  // Should be -1 (all bits flipped)
        print("NOT: " + notResult);

        // Test operator precedence: & has higher precedence than |
        // a & b | c should be (a & b) | c, not a & (b | c)
        int f = 1;
        int g = 2;
        int h = 4;
        int precedenceTest = f | g & h;  // Should be 1 | (2 & 4) = 1 | 0 = 1
        print("Precedence (1 | 2 & 4): " + precedenceTest);

        // Test shift precedence: << has higher precedence than &
        int shiftPrecedence = 1 << 2 & 12;  // Should be (1 << 2) & 12 = 4 & 12 = 4
        print("Shift precedence (1 << 2 & 12): " + shiftPrecedence);

        // Test chained operations
        int chained = a & b | c ^ d;
        print("Chained: " + chained);

        // Test with parentheses to override precedence
        int withParens = (a | b) & c;
        print("With parens ((12 | 10) & 3): " + withParens);

        // Test double negation
        int doubleNot = ~~5;  // Should be 5
        print("Double NOT: " + doubleNot);
    }

main();