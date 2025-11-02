import * from "../../../lib/primitives/Int.mt";
import * from "../../../lib/primitives/String.mt";

// Test self-referencing generic types (like Comparable<T extends Comparable<T>>)
interface Comparable<T> {
    public function compareTo(T other): int;
}

interface Cloneable<T> {
    public function clone(): T;
}

// Self-referencing implementation
class Number implements Comparable<Number>, Cloneable<Number> {
    int value;

    constructor(int v) {
        value = v;
    }

    public function compareTo(Number other): int {
        if (value < other.value) {
            return -1;
        } else if (value > other.value) {
            return 1;
        }
        return 0;
    }

    public function clone(): Number {
        return new Number(value);
    }

    public function getValue(): int {
        return value;
    }

    public function setValue(int v): void {
        value = v;
    }
}

class Text implements Comparable<Text>, Cloneable<Text> {
    string content;

    constructor(string c) {
        content = c;
    }

    public function compareTo(Text other): int {
        // Simple length comparison for demonstration
        int thisLen = 0;
        int otherLen = 0;

        // Count characters (simplified)
        if (content == other.content) {
            return 0;
        }
        return 1;  // Simplified comparison
    }

    public function clone(): Text {
        return new Text(content);
    }

    public function getContent(): string {
        return content;
    }
}

// Generic function with self-referencing bound
function <T extends Comparable<T>> findMax(T a, T b): T {
    if (a.compareTo(b) >= 0) {
        return a;
    }
    return b;
}

function <T extends Cloneable<T>> duplicate(T item): T {
    return item.clone();
}

function main(): void {
    print("Testing self-referencing generic types");

    Number num1 = new Number(42);
    Number num2 = new Number(17);
    Number num3 = new Number(99);

    print("num1 value: " + num1.getValue());
    print("num2 value: " + num2.getValue());
    print("num3 value: " + num3.getValue());

    // Test compareTo
    int cmp1 = num1.compareTo(num2);
    print("num1 vs num2: " + cmp1);

    int cmp2 = num2.compareTo(num3);
    print("num2 vs num3: " + cmp2);

    // Test findMax with self-referencing bound
    Number maxNum = findMax<Number>(num1, num2);
    print("max(num1, num2): " + maxNum.getValue());

    maxNum = findMax<Number>(num2, num3);
    print("max(num2, num3): " + maxNum.getValue());

    // Test clone
    Number cloned1 = duplicate<Number>(num1);
    print("Cloned num1: " + cloned1.getValue());

    cloned1.setValue(999);
    print("Modified clone: " + cloned1.getValue());
    print("Original num1: " + num1.getValue());

    // Test with Text
    Text text1 = new Text("Hello");
    Text text2 = new Text("World");

    Text maxText = findMax<Text>(text1, text2);
    print("Max text: " + maxText.getContent());

    Text clonedText = duplicate<Text>(text1);
    print("Cloned text: " + clonedText.getContent());

    print("Self-referencing generic test completed");
}

main();
