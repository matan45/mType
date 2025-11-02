// Test bounded generic types beyond simple extends
// @Script

interface Comparable<T> {
    func compareTo(other: T): Int;
}

class Number implements Comparable<Number> {
    var value: Int;

    func init(value: Int) {
        this.value = value;
    }

    func compareTo(other: Number): Int {
        if (this.value < other.value) {
            return -1;
        }
        if (this.value > other.value) {
            return 1;
        }
        return 0;
    }
}

interface Container<T extends Comparable<T>> {
    func add(item: T): void;
    func findMax(): T;
}

class NumberContainer implements Container<Number> {
    var items: Array<Number>;

    func init() {
        this.items = new Array<Number>();
    }

    func add(item: Number): void {
        this.items.add(item);
    }

    func findMax(): Number {
        if (this.items.size() == 0) {
            return new Number(0);
        }

        var max = this.items.get(0);
        var i = 1;
        while (i < this.items.size()) {
            var current = this.items.get(i);
            if (current.compareTo(max) > 0) {
                max = current;
            }
            i = i + 1;
        }
        return max;
    }
}

var container = new NumberContainer();
container.add(new Number(5));
container.add(new Number(10));
container.add(new Number(3));

var max = container.findMax();
print(max.value);  // Should print 10
