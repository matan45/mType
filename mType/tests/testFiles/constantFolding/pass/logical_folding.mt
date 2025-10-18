// Test constant folding for logical operations

function testLogicalOperations() {
    // AND operations
    bool a = true && true;      // Should fold to true
    bool b = true && false;     // Should fold to false
    bool c = false && true;     // Should fold to false
    bool d = false && false;    // Should fold to false

    assert(a == true);
    assert(b == false);
    assert(c == false);
    assert(d == false);

    // OR operations
    bool e = true || true;      // Should fold to true
    bool f = true || false;     // Should fold to true
    bool g = false || true;     // Should fold to true
    bool h = false || false;    // Should fold to false

    assert(e == true);
    assert(f == true);
    assert(g == true);
    assert(h == false);

    // NOT operations
    bool i = !true;             // Should fold to false
    bool j = !false;            // Should fold to true

    assert(i == false);
    assert(j == true);

    return 0;
}

function testComplexLogicalExpressions() {
    // Nested logical expressions
    bool a = (true && true) || false;       // Should fold to true
    bool b = (false || false) && true;      // Should fold to false
    bool c = !(true && false);              // Should fold to true
    bool d = !(!true);                      // Should fold to true

    assert(a == true);
    assert(b == false);
    assert(c == true);
    assert(d == true);

    return 0;
}

function testLogicalWithComparisons() {
    // Logical operations combined with comparisons
    bool a = (5 > 3) && (10 < 20);         // Should fold to true
    bool b = (5 == 5) || (3 > 10);         // Should fold to true
    bool c = !(5 < 3);                     // Should fold to true

    assert(a == true);
    assert(b == true);
    assert(c == true);

    return 0;
}

function main() {
    testLogicalOperations();
    testComplexLogicalExpressions();
    testLogicalWithComparisons();

    print("All logical folding tests passed!");
    return 0;
}
