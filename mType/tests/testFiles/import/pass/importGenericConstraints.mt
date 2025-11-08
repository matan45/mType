// Test: Import generic with type constraints
import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/String.mt";
import { SortedBox, Comparable } from "modules/GenericConstraintsModule.mt";

class Number implements Comparable<Number> {
    public int value;

    constructor(int v) {
        this.value = v;
    }

    public function compareTo(Number other): int {
        return this.value - other.value;
    }
}

SortedBox<Number> box = new SortedBox<Number>();
box.add(new Number(5));
box.add(new Number(3));
print(box.getCount());
