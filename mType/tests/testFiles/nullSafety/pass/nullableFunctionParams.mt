// Test: Nullable types in function parameters and return types

class Box {
    public int value;
    constructor(int v) {
        this.value = v;
    }
    public function getValue(): int {
        return this.value;
    }
}

// Function with nullable parameter
function describe(Box? box): string {
    if (box != null) {
        return "Box(" + box.getValue() + ")";
    }
    return "null";
}

// Function with nullable return type
function findBox(int target): Box? {
    if (target > 0) {
        return new Box(target);
    }
    return null;
}

// Function accepting non-nullable and nullable params
function combine(Box first, Box? second): string {
    string result = "first=" + first.getValue();
    if (second != null) {
        result = result + ", second=" + second.getValue();
    } else {
        result = result + ", second=null";
    }
    return result;
}

// Test nullable parameter with object
print(describe(new Box(42)));

// Test nullable parameter with null
print(describe(null));

// Test nullable return — returns object
Box? found = findBox(10);
print("found: " + (found != null));
if (found != null) {
    print("found value: " + found.getValue());
}

// Test nullable return — returns null
Box? notFound = findBox(-1);
print("notFound: " + (notFound == null));

// Test mixed params
Box nonNull = new Box(1);
print(combine(nonNull, new Box(2)));
print(combine(nonNull, null));

print("Nullable function params tests passed!");
