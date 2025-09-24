// Test object type mismatch in array literals
class Person {
    string name;
    constructor(string n) { name = n; }
}

class Car {
    string model;
    constructor(string m) { model = m; }
}

print("Testing object type mismatch");

// Object type mismatch
Person[] people = [
    new Person("Alice"),
    new Car("Toyota"),     // Error: Car in Person array
    new Person("Bob")
];

print("This should not be reached");