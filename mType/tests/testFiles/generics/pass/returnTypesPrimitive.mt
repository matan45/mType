// Test primitive return types
function getInt(): int {
    return 42;
}

function getFloat(): float {
    return 3.14;
}

function getBool(): bool {
    return true;
}

function getString(): string {
    return "hello";
}

function getVoid(): void {
    print("void function");
}

// Test functions
print(getInt());
print(getFloat());
print(getBool());
print(getString());
getVoid();
