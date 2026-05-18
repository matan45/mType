// Test: a nullable Object receiver narrows and the narrowed value can be
// passed to a non-nullable Object parameter.

class Object {}

class Box extends Object {
    public int value;
    constructor(int v) {
        this.value = v;
    }
}

function describe(Object o): string {
    return "got an object";
}

Object? maybe = new Box(42);
if (maybe != null) {
    print(describe(maybe));
}

Object? empty = null;
if (empty == null) {
    print("empty is null");
} else {
    print("should not reach here: " + describe(empty));
}

print("Nullable Object receiver tests passed!");
