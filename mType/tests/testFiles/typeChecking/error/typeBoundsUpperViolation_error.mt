import * from "../../../lib/primitives/Int.mt";
import * from "../../../lib/primitives/String.mt";

// Test: Upper bound constraint violation
// Expected: Error - type parameter violates upper bound constraint

interface Comparable<T> {
    function compareTo(T other): int;
}

class Number implements Comparable<Number> {
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

    public function getValue(): int {
        return value;
    }
}

class Text {
    string content;

    constructor(string c) {
        content = c;
    }

    public function getContent(): string {
        return content;
    }
}

// Generic class with upper bound constraint
class BoundedBox<T extends Comparable<T>> {
    T item;

    constructor(T i) {
        item = i;
    }

    public function getItem(): T {
        return item;
    }

    public function compareTo(T other): int {
        return item.compareTo(other);
    }
}

function main(): void {
    Number num = new Number(42);
    BoundedBox<Number> numberBox = new BoundedBox<Number>(num);
    print("Number box created: " + numberBox.getItem().getValue());

    // ERROR: Text does not implement Comparable<Text>
    // This violates the upper bound constraint T extends Comparable<T>
    Text text = new Text("Hello");
    BoundedBox<Text> textBox = new BoundedBox<Text>(text);
    print("Text box created: " + textBox.getItem().getContent());
}

main();
