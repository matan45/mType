import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/String.mt";

// Test self-referencing generic constraints like <T extends Comparable<T>>
interface Comparable<T> {
    function compareTo(T other): int;
}

class ComparableInt implements Comparable<ComparableInt> {
    int value;

    public function ComparableInt(int val) {
        value = val;
    }

    public function compareTo(ComparableInt other): int {
        if (value < other.value) {
            return -1;
        } else if (value > other.value) {
            return 1;
        }
        return 0;
    }

    public function getValue(): int {
        return value;
    }
}

function <T extends Comparable<T>> max(T a, T b): T {
    if (a.compareTo(b) > 0) {
        return a;
    }
    return b;
}

function <T extends Comparable<T>> min(T a, T b): T {
    if (a.compareTo(b) < 0) {
        return a;
    }
    return b;
}

function main(): void {
    print("Testing self-referencing generic constraints");

    ComparableInt num1 = new ComparableInt(42);
    ComparableInt num2 = new ComparableInt(17);
    ComparableInt num3 = new ComparableInt(99);

    print("num1 value: " + num1.getValue());
    print("num2 value: " + num2.getValue());
    print("num3 value: " + num3.getValue());

    // Test compareTo
    int result1 = num1.compareTo(num2);
    print("num1.compareTo(num2): " + result1);

    int result2 = num2.compareTo(num3);
    print("num2.compareTo(num3): " + result2);

    // Test generic max function
    ComparableInt maxVal = max<ComparableInt>(num1, num2);
    print("max(num1, num2): " + maxVal.getValue());

    maxVal = max<ComparableInt>(num2, num3);
    print("max(num2, num3): " + maxVal.getValue());

    // Test generic min function
    ComparableInt minVal = min<ComparableInt>(num1, num2);
    print("min(num1, num2): " + minVal.getValue());

    minVal = min<ComparableInt>(num1, num3);
    print("min(num1, num3): " + minVal.getValue());

    print("Self-referencing constraint test completed");
}

main();
