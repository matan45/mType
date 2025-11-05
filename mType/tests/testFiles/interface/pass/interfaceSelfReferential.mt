// Test self-referential generic interface (Comparable pattern)
// @Script

interface Comparable<T extends Comparable<T>> {
    function compareTo(T other): int;
}

class Person implements Comparable<Person> {
    private int age;
    private string name;

    public function init(string name, int age) {
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

class Employee extends Person {
    private int salary;

    public function init(string name, int age, int salary) {
        super.init(name, age);
        this.salary = salary;
    }

    // Inherited compareTo works with Employee as well
}

function findOlder<T extends Comparable<T>>(T a, T b): T {
    if (a.compareTo(b) > 0) {
        return a;
    }
    return b;
}

Person p1 = new Person("Alice", 30);
Person p2 = new Person("Bob", 25);

Person older = findOlder<Person>(p1, p2);
print(older.name);  // Should print "Alice"

Employee e1 = new Employee("Charlie", 35, 50000);
Employee e2 = new Employee("Diana", 40, 60000);

Employee olderEmployee = findOlder<Employee>(e1, e2);
print(olderEmployee.name);  // Should print "Diana"
