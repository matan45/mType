// Test primitive type parameters
function testInt(int x): int {
    return x + 1;
}

function testFloat(float f): float {
    return f * 2.0;
}

function testBool(bool b): bool {
    return !b;
}

function testString(string s): string {
    return s;
}

function testMultiplePrimitives(int a, float b, bool c, string d): void {
    print(a);
    print(b);
    print(c);
    print(d);
}

// Call functions
print(testInt(5));
print(testFloat(2.5));
print(testBool(true));
print(testString("hello"));
testMultiplePrimitives(10, 3.14, false, "world");
