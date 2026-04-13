// Test: Null safety enforcement for parameters and return types

class Box {
    public int value;
    constructor(int v) {
        this.value = v;
    }
    public function getValue(): int {
        return this.value;
    }
}

// === Nullable parameter accepts null ===
function describeBox(Box? b): string {
    if (b != null) {
        return "Box(" + b.getValue() + ")";
    }
    return "null";
}

print(describeBox(new Box(10)));
print(describeBox(null));

// === Non-nullable parameter accepts non-nullable value ===
function processBox(Box b): int {
    return b.getValue();
}

Box nonNull = new Box(42);
print("process: " + processBox(nonNull));

// === Nullable return type can return null ===
function findBox(int id): Box? {
    if (id > 0) {
        return new Box(id);
    }
    return null;
}

Box? found = findBox(5);
if (found != null) {
    print("found: " + found.getValue());
}

Box? missing = findBox(-1);
print("missing is null: " + (missing == null));

// === Smart cast: narrowed nullable passed to non-nullable param ===
function useBox(Box b): string {
    return "value=" + b.getValue();
}

Box? maybe = new Box(99);
if (maybe != null) {
    print("narrowed: " + useBox(maybe));
}

// === Non-nullable return type with non-null expression ===
function createBox(int v): Box {
    return new Box(v);
}

Box created = createBox(7);
print("created: " + created.getValue());

// === Nullable param forwarded to nullable param ===
function wrapDescribe(Box? b): string {
    return describeBox(b);
}

print(wrapDescribe(new Box(3)));
print(wrapDescribe(null));

print("Null safe param and return tests passed!");
