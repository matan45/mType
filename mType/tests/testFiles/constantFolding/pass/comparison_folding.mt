// Test constant folding for comparison operations

function testIntegerComparisons(): void {
    // Equality comparisons
    bool a = (5 == 5);      // Should fold to true
    bool b = (5 == 3);      // Should fold to false
    bool c = (5 != 3);      // Should fold to true
    bool d = (5 != 5);      // Should fold to false

    print("Testing: (5 == 5) = " + a + " (expected true)");
    print("Testing: (5 == 3) = " + b + " (expected false)");
    print("Testing: (5 != 3) = " + c + " (expected true)");
    print("Testing: (5 != 5) = " + d + " (expected false)");

    // Relational comparisons
    bool e = (5 < 10);      // Should fold to true
    bool f = (10 < 5);      // Should fold to false
    bool g = (5 <= 5);      // Should fold to true
    bool h = (5 > 3);       // Should fold to true
    bool i = (3 > 5);       // Should fold to false
    bool j = (5 >= 5);      // Should fold to true

    print("Testing: (5 < 10) = " + e + " (expected true)");
    print("Testing: (10 < 5) = " + f + " (expected false)");
    print("Testing: (5 <= 5) = " + g + " (expected true)");
    print("Testing: (5 > 3) = " + h + " (expected true)");
    print("Testing: (3 > 5) = " + i + " (expected false)");
    print("Testing: (5 >= 5) = " + j + " (expected true)");

    print("Integer comparisons tests completed\n");
}

function testFloatComparisons(): void {
    // Float comparisons
    bool a = (2.5 == 2.5);  // Should fold to true
    bool b = (2.5 != 3.5);  // Should fold to true
    bool c = (2.5 < 3.5);   // Should fold to true
    bool d = (3.5 > 2.5);   // Should fold to true

    print("Testing: (2.5 == 2.5) = " + a + " (expected true)");
    print("Testing: (2.5 != 3.5) = " + b + " (expected true)");
    print("Testing: (2.5 < 3.5) = " + c + " (expected true)");
    print("Testing: (3.5 > 2.5) = " + d + " (expected true)");

    print("Float comparisons tests completed\n");
}

function testStringComparisons(): void {
    // String comparisons
    bool a = ("hello" == "hello");  // Should fold to true
    bool b = ("hello" != "world");  // Should fold to true
    bool c = ("abc" < "def");       // Should fold to true (lexicographic)
    bool d = ("xyz" > "abc");       // Should fold to true

    print("Testing: (\"hello\" == \"hello\") = " + a + " (expected true)");
    print("Testing: (\"hello\" != \"world\") = " + b + " (expected true)");
    print("Testing: (\"abc\" < \"def\") = " + c + " (expected true)");
    print("Testing: (\"xyz\" > \"abc\") = " + d + " (expected true)");

    print("String comparisons tests completed\n");
}

function testBooleanComparisons(): void {
    // Boolean comparisons
    bool a = (true == true);    // Should fold to true
    bool b = (true == false);   // Should fold to false
    bool c = (true != false);   // Should fold to true

    print("Testing: (true == true) = " + a + " (expected true)");
    print("Testing: (true == false) = " + b + " (expected false)");
    print("Testing: (true != false) = " + c + " (expected true)");

    print("Boolean comparisons tests completed\n");
}

function main(): void {
    print("=== Comparison Constant Folding Tests ===\n");

    testIntegerComparisons();
    testFloatComparisons();
    testStringComparisons();
    testBooleanComparisons();

    print("=== All comparison folding tests completed! ===");
}

main();
