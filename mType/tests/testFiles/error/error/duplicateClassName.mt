// Error: Duplicate class name declaration

class Person {
    string name;

    constructor(string n) {
        this.name = n;
    }
}

// ERROR: Duplicate class with same name
class Person {
    int age;

    constructor(int a) {
        this.age = a;
    }
}
