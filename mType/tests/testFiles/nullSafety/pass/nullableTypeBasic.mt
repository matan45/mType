// Test: Basic nullable type declarations and assignments

class Box {
    public int value;
    constructor(int v) {
        this.value = v;
    }
    public function getValue(): int {
        return this.value;
    }
}

// Nullable variable can hold null
Box? nullable1 = null;
print("nullable1 is null: " + (nullable1 == null));

// Nullable variable can hold an object
Box? nullable2 = new Box(42);
print("nullable2 is null: " + (nullable2 == null));
print("nullable2 value: " + nullable2.getValue());

// Non-nullable variable must hold an object
Box nonNull = new Box(10);
print("nonNull value: " + nonNull.getValue());

// Nullable can be reassigned to null
nullable2 = null;
print("nullable2 after null: " + (nullable2 == null));

// Nullable can be reassigned to object
nullable1 = new Box(99);
print("nullable1 after assign: " + nullable1.getValue());

print("All nullable type tests passed!");
