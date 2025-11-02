// Test lambda type inference with interface
// @Script

interface Comparator<T> {
    func compare(a: T, b: T): Int;
}

class Sorter<T> {
    var items: Array<T>;

    func init(items: Array<T>) {
        this.items = items;
    }

    func sort(comparator: Comparator<T>): void {
        var n = this.items.size();
        var i = 0;
        while (i < n - 1) {
            var j = 0;
            while (j < n - i - 1) {
                var a = this.items.get(j);
                var b = this.items.get(j + 1);
                if (comparator.compare(a, b) > 0) {
                    this.items.set(j, b);
                    this.items.set(j + 1, a);
                }
                j = j + 1;
            }
            i = i + 1;
        }
    }
}

class LambdaComparator<T> implements Comparator<T> {
    var fn: func(T, T): Int;

    func init(fn: func(T, T): Int) {
        this.fn = fn;
    }

    func compare(a: T, b: T): Int {
        return this.fn(a, b);
    }
}

var numbers = new Array<Int>();
numbers.add(5);
numbers.add(2);
numbers.add(8);
numbers.add(1);
numbers.add(9);

var sorter = new Sorter<Int>(numbers);

// Lambda with inferred types
var ascendingComparator = new LambdaComparator<Int>(func(a: Int, b: Int): Int {
    if (a < b) {
        return -1;
    }
    if (a > b) {
        return 1;
    }
    return 0;
});

sorter.sort(ascendingComparator);

print("Sorted:");
var i = 0;
while (i < numbers.size()) {
    print(numbers.get(i));
    i = i + 1;
}
