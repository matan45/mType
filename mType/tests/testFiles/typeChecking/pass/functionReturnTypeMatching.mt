// Test functions with different return types
function getInt(): int {
    return 100;
}

function getString(): string {
    return "test";
}

function getBool(): bool {
    return false;
}

function getFloat(): float {
    return 2.5;
}

// Test valid assignments - matching types
int validInt = getInt();
string validString = getString();
bool validBool = getBool();
float validFloat = getFloat();

// Test allowed conversions
float floatFromInt = getInt();  // Int to float conversion is allowed

// Test function calls in expressions
int result = getInt() + 10;
string message = getString() + " world";
bool flag = getBool() || true;

// Test nested function calls
function processInt(int value): int {
    return value * 2;
}

int nested = processInt(getInt());

print("All return type matching tests passed");