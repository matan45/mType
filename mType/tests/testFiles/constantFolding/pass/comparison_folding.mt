// Test constant folding for comparison operations

function testIntegerComparisons() {
    // Equality comparisons
    bool a = (5 == 5);      // Should fold to true
    bool b = (5 == 3);      // Should fold to false
    bool c = (5 != 3);      // Should fold to true
    bool d = (5 != 5);      // Should fold to false

    assert(a == true);
    assert(b == false);
    assert(c == true);
    assert(d == false);

    // Relational comparisons
    bool e = (5 < 10);      // Should fold to true
    bool f = (10 < 5);      // Should fold to false
    bool g = (5 <= 5);      // Should fold to true
    bool h = (5 > 3);       // Should fold to true
    bool i = (3 > 5);       // Should fold to false
    bool j = (5 >= 5);      // Should fold to true

    assert(e == true);
    assert(f == false);
    assert(g == true);
    assert(h == true);
    assert(i == false);
    assert(j == true);

    return 0;
}

function testFloatComparisons() {
    // Float comparisons
    bool a = (2.5 == 2.5);  // Should fold to true
    bool b = (2.5 != 3.5);  // Should fold to true
    bool c = (2.5 < 3.5);   // Should fold to true
    bool d = (3.5 > 2.5);   // Should fold to true

    assert(a == true);
    assert(b == true);
    assert(c == true);
    assert(d == true);

    return 0;
}

function testStringComparisons() {
    // String comparisons
    bool a = ("hello" == "hello");  // Should fold to true
    bool b = ("hello" != "world");  // Should fold to true
    bool c = ("abc" < "def");       // Should fold to true (lexicographic)
    bool d = ("xyz" > "abc");       // Should fold to true

    assert(a == true);
    assert(b == true);
    assert(c == true);
    assert(d == true);

    return 0;
}

function testBooleanComparisons() {
    // Boolean comparisons
    bool a = (true == true);    // Should fold to true
    bool b = (true == false);   // Should fold to false
    bool c = (true != false);   // Should fold to true

    assert(a == true);
    assert(b == false);
    assert(c == true);

    return 0;
}

function main() {
    testIntegerComparisons();
    testFloatComparisons();
    testStringComparisons();
    testBooleanComparisons();

    print("All comparison folding tests passed!");
    return 0;
}
