// Returning null for object types test
import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/String.mt";

interface Function<T, R> {
    function apply(T input) : R;
}

class Box {
    int value;

    constructor(int v) {
        this.value = v;
    }

    public function getValue() : int {
        return this.value;
    }
}

print("=== Null Return Test ===");

// Lambda returning null for object type
Function<Int, String?> nullReturner = x -> {
    if (x < 0) {
        return null;
    } else {
        return "Value: " + x;
    }
};

String? result1 = nullReturner.apply(5);
String? result2 = nullReturner.apply(-1);

print("Result for 5: " + (result1 == null ? "null" : result1));
print("Result for -1: " + (result2 == null ? "null" : result2));

// Lambda returning null objects
Function<Int, Box?> boxCreator = x -> {
    if (x.getValue() == 0) {
        return null;
    } else {
        return new Box(x.getValue());
    }
};

Box? b1 = boxCreator.apply(10);
Box? b2 = boxCreator.apply(0);

print("Box for 10: " + (b1 == null ? "null" : b1.getValue()));
print("Box for 0: " + (b2 == null ? "null" : "not null"));

// Lambda with null check
Function<String, Int> safeLength = s -> {
    if (s == null) {
        return 0;
    } else {
        return s.length();
    }
};

print("Length of 'hello': " + safeLength.apply("hello"));
print("Length of null: " + safeLength.apply(null));

print("Null return complete");
