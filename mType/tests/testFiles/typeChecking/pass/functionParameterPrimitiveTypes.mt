function testInt(int a, int b): int {
    return a + b;
}

function testString(string s1, string s2): string {
    return s1 + s2;
}

function testBool(bool flag): bool {
    return !flag;
}

function testMixed(int n, string s, bool b): string {
    if (b) {
        return s + toString(n);
    }
    return s;
}

// Test correct calls
int result1 = testInt(5, 10);
string result2 = testString("Hello", " World");
bool result3 = testBool(true);
string result4 = testMixed(42, "Answer: ", true);

print(toString(result1));
print(result2);
print(toString(result3));
print(result4);