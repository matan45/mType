// Test bytecode-level patterns that AST optimizer can't optimize

function testLocalVar(): int {
    int x = 10;
    int y = x;  // Will generate STORE_LOCAL + LOAD_LOCAL
    return y;
}

function testMultipleStores(): int {
    int a = 5;
    int b = a;  // STORE_LOCAL + LOAD_LOCAL
    int c = b;  // STORE_LOCAL + LOAD_LOCAL
    return c;
}

function testNestedCalls(): int {
    int x = testLocalVar();
    return x;
}

print(testLocalVar());
print(testMultipleStores());
print(testNestedCalls());
