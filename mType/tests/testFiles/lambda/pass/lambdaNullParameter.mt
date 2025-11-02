// Passing null to lambda parameters test
interface Function<T, R> {
    function apply(T input) : R;
}

print("=== Null Parameter Test ===");

// Lambda handling null string
Function<String, int> safeLength = s -> {
    if (s == null) {
        return 0;
    } else {
        return s.length();
    }
};

print("Length of 'hello': " + safeLength.apply("hello"));
print("Length of null: " + safeLength.apply(null));
print("Length of '': " + safeLength.apply(""));

// Lambda with null object
class Box {
    int value;

    function init(int v) {
        this.value = v;
    }

    function getValue() : int {
        return this.value;
    }
}

Function<Box, int> safeExtractor = box -> {
    if (box == null) {
        return -1;
    } else {
        return box.getValue();
    }
};

Box b1 = new Box(42);
print("Extract from box: " + safeExtractor.apply(b1));
print("Extract from null: " + safeExtractor.apply(null));

// Lambda with null array
Function<int[], int> arraySum = arr -> {
    if (arr == null) {
        return 0;
    }
    int sum = 0;
    for (int i = 0; i < arr.length; i = i + 1) {
        sum = sum + arr[i];
    }
    return sum;
};

int[] nums = [1, 2, 3, 4, 5];
print("Sum of array: " + arraySum.apply(nums));
print("Sum of null: " + arraySum.apply(null));

// Lambda returning null or value
Function<bool, String> conditionalString = flag -> {
    if (flag) {
        return "Not null";
    } else {
        return null;
    }
};

String r1 = conditionalString.apply(true);
String r2 = conditionalString.apply(false);
print("True result: " + (r1 == null ? "null" : r1));
print("False result: " + (r2 == null ? "null" : r2));

print("Null parameter complete");
