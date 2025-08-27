namespace types {
int intValue = 42;
bool boolValue = true;
string stringValue = "Hello";

function intFunction(int x): int {
    return x * 2;
}

function boolFunction(bool x): bool {
    return !x;
}

function stringFunction(string s): string {
    return s + " World";
}

function getIntValue(): int {
    return intValue;
}

function getBoolValue(): bool {
    return boolValue;
}

function getStringValue(): string {
    return stringValue;
}

namespace mixed {
    int count = 10;
    string message = "Test";
    bool active = true;
    
    function process(int n): int {
        return n + count;
    }
    
    function getMessage(): string {
        return message;
    }
    
    function isActive(): bool {
        return active;
    }
}
}

// Test functions with different return types
int doubled = types::intFunction(25);
print(doubled);

bool negated = types::boolFunction(false);
print(negated);

int originalInt = types::getIntValue();
print(originalInt);

bool originalBool = types::getBoolValue();
print(originalBool);

int processed = types::mixed::process(5);
print(processed);

bool active = types::mixed::isActive();
print(active);