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
}

function main(): void {
    // Test SimpleContainer with custom classes
    SimpleContainer<Person> personContainer = new SimpleContainer<Person>();
    SimpleContainer<Employee> employeeContainer = new SimpleContainer<Employee>();

    Person alice = new Person("Alice", 25);
    Employee john = new Employee("John", "Engineering");

    personContainer.store(alice);
    employeeContainer.store(john);

    print("Person empty: " + personContainer.isEmpty());
    print("Employee empty: " + employeeContainer.isEmpty());

    Person retrievedPerson = personContainer.retrieve();
    Employee retrievedEmployee = employeeContainer.retrieve();

    print("Retrieved person: " + retrievedPerson.toString());
    print("Retrieved employee: " + retrievedEmployee.toString());

    // Test Validator with custom classes
    Validator<Person> personValidator = new Validator<Person>();
    Validator<Employee> employeeValidator = new Validator<Employee>();

    print(personValidator.validateAndProcess(alice));
    print(employeeValidator.validateAndProcess(john));
}

main();