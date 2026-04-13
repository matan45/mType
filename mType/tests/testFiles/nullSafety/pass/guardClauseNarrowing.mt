// Test: Guard-clause narrowing — if (x == null) { return/throw; } narrows x

class Box {
    public int value;

    constructor(int v) {
        this.value = v;
    }

    public function getValue(): int {
        return this.value;
    }
}

// Test 1: Guard clause with return
function testReturnGuard(Box? b): string {
    if (b == null) {
        return "null";
    }
    return "value: " + b.getValue();
}

// Test 2: Multiple sequential guards
function testMultipleGuards(Box? a, Box? b): int {
    if (a == null) {
        return -1;
    }
    if (b == null) {
        return -2;
    }
    return a.getValue() + b.getValue();
}

// Test 3: Guard clause with field access
function testFieldAccess(Box? b): int {
    if (b == null) {
        return 0;
    }
    return b.value;
}

print("Test 1: Guard with return");
print(testReturnGuard(null));
print(testReturnGuard(new Box(42)));

print("Test 2: Multiple guards");
print(testMultipleGuards(new Box(10), new Box(20)));
print(testMultipleGuards(null, new Box(20)));
print(testMultipleGuards(new Box(10), null));

print("Test 3: Field access after guard");
print(testFieldAccess(null));
print(testFieldAccess(new Box(99)));

print("Guard clause tests passed!");
