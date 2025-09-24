// Test: Function names must start with lowercase letter
// Expected Error: Function 'GetValue' must start with a lowercase letter. Functions and methods should follow camelCase convention.

function GetValue(): int {
    return 42;
}

function main(): void {
    int value = GetValue();
    print(value);
}