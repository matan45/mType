// Test: Import generic with type constraints
@Script
import { SortedBox, Comparable } from "./modules/GenericConstraintsModule.mt"

class Number implements Comparable<Number> {
    var value: Int;

    constructor(v: Int) {
        this.value = v;
    }

    fun compareTo(other: Number): Int {
        return this.value - other.value;
    }
}

var box = SortedBox<Number>();
box.add(Number(5));
box.add(Number(3));
print(box.getCount());
