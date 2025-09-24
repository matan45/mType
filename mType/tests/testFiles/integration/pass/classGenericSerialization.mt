// Test generic serialization with custom classes
import "genericLibrary.mt";

class Person {
    string name;
    int age;

    constructor(string n, int a) {
        name = n;
        age = a;
    }

    function getName(): string {
        return name;
    }

    function getAge(): int {
        return age;
    }

    function toString(): string {
        return name + " (" + age + " years old)";
    }

    function equals(Person other): bool {
        return this.name == other.name && this.age == other.age;
    }

    function hashCode(): int {
        return hashCode(name) + age;
    }
}

class Employee {
    string name;
    string department;

    constructor(string n, string d) {
        name = n;
        department = d;
    }

    function toString(): string {
        return name + " from " + department;
    }

    function equals(Employee other): bool {
        return this.name == other.name && this.department == other.department;
    }

    function hashCode(): int {
        return hashCode(name) + hashCode(department);
    }
}

function main(): void {
    // Test GenericContainer with custom classes
    GenericContainer<Person> personContainer = new GenericContainer<Person>();
    GenericContainer<Employee> employeeContainer = new GenericContainer<Employee>();

    Person alice = new Person("Alice", 25);
    Employee john = new Employee("John", "Engineering");

    personContainer.add(alice);
    employeeContainer.add(john);

    print("Person size: " + personContainer.size());
    print("Employee size: " + employeeContainer.size());

    Person retrievedPerson = personContainer.getFirst();
    Employee retrievedEmployee = employeeContainer.getFirst();

    print("Retrieved person: " + retrievedPerson.toString());
    print("Retrieved employee: " + retrievedEmployee.toString());

    // Test contains functionality
    print("Contains alice: " + personContainer.contains(alice));
    print("Contains john: " + employeeContainer.contains(john));
}

main();