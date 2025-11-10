// Test collection of interface type
// @Script

import * from "../../lib/collections/List.mt";

interface Comparable<T> {
    function compareTo(T other): int;
}

class Person implements Comparable<Person> {
    public string name;
    public int age;

    public constructor(string name, int age) {
        this.name = name;
        this.age = age;
    }

    public function compareTo(Person other): int {
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
    private List<T> items;

    public constructor() {
        this.items = new List<T>();
    }

    public function add(T item): void {
        this.items.add(item);
    }

    public function sort(): void {
        // Simple bubble sort
        int n = this.items.size();
        for (int i = 0; i < n - 1; i++) {
            for (int j = 0; j < n - i - 1; j++) {
                T current = this.items.get(j);
                T next = this.items.get(j + 1);
                if (current.compareTo(next) > 0) {
                    // Swap
                    this.items.set(j, next);
                    this.items.set(j + 1, current);
                }
            }
        }
    }

    public function print(): void {
        for (int i = 0; i < this.items.size(); i++) {
            Person person = this.items.get(i);
            print(person.name + " (" + person.age + ")");
        }
    }
}

ComparableList<Person> people = new ComparableList<Person>();
people.add(new Person("Alice", 30));
people.add(new Person("Bob", 25));
people.add(new Person("Charlie", 35));

print("Before sorting:");
people.print();

people.sort();

print("After sorting:");
people.print();
