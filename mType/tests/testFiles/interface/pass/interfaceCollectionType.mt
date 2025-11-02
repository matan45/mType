// Test collection of interface type
// @Script

interface Comparable<T> {
    func compareTo(other: T): Int;
}

class Person implements Comparable<Person> {
    var name: String;
    var age: Int;

    func init(name: String, age: Int) {
        this.name = name;
        this.age = age;
    }

    func compareTo(other: Person): Int {
        if (this.age < other.age) {
            return -1;
        }
        if (this.age > other.age) {
            return 1;
        }
        return 0;
    }
}

// Collection of comparable items
class ComparableList<T extends Comparable<T>> {
    var items: Array<T>;

    func init() {
        this.items = new Array<T>();
    }

    func add(item: T): void {
        this.items.add(item);
    }

    func sort(): void {
        // Simple bubble sort
        var n = this.items.size();
        var i = 0;
        while (i < n - 1) {
            var j = 0;
            while (j < n - i - 1) {
                var current = this.items.get(j);
                var next = this.items.get(j + 1);
                if (current.compareTo(next) > 0) {
                    // Swap
                    this.items.set(j, next);
                    this.items.set(j + 1, current);
                }
                j = j + 1;
            }
            i = i + 1;
        }
    }

    func print(): void {
        var i = 0;
        while (i < this.items.size()) {
            var person = this.items.get(i);
            print(person.name + " (" + person.age.toString() + ")");
            i = i + 1;
        }
    }
}

var people = new ComparableList<Person>();
people.add(new Person("Alice", 30));
people.add(new Person("Bob", 25));
people.add(new Person("Charlie", 35));

print("Before sorting:");
people.print();

people.sort();

print("After sorting:");
people.print();
