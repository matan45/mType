// ERROR: Test incompatible element type assignment to array

class Person {
    string name;
    constructor(string n) {
        name = n;
    }
}

class Company {
    string companyName;
    constructor(string cn) {
        companyName = cn;
    }
}

function main(): void {
    // Create array of Person objects
    Person[] people = new Person[3];
    people[0] = new Person("Alice");
    people[1] = new Person("Bob");

    // ERROR: Cannot assign Company to Person[] element
    people[2] = new Company("TechCorp");

    print("This should not execute");
}

main();
