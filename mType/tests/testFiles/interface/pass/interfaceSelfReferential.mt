// Test self-referential generic interface (Comparable pattern)
// @Script

interface Comparable<T extends Comparable<T>> {
    func compareTo(other: T): Int;
}

class Person implements Comparable<Person> {
    var age: Int;
    var name: String;

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

class Employee extends Person {
    var salary: Int;

    func init(name: String, age: Int, salary: Int) {
        super.init(name, age);
        this.salary = salary;
    }

    // Inherited compareTo works with Employee as well
}

func findOlder<T extends Comparable<T>>(a: T, b: T): T {
    if (a.compareTo(b) > 0) {
        return a;
    }
    return b;
}

var p1 = new Person("Alice", 30);
var p2 = new Person("Bob", 25);

var older = findOlder<Person>(p1, p2);
print(older.name);  // Should print "Alice"

var e1 = new Employee("Charlie", 35, 50000);
var e2 = new Employee("Diana", 40, 60000);

var olderEmployee = findOlder<Employee>(e1, e2);
print(olderEmployee.name);  // Should print "Diana"
