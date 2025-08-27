class Person {
    string name;
    int age;
    
    constructor(string n, int a) {
        name = n;
        age = a;
    }
}

Person p = new Person(42, "John");  // Wrong order: int, string instead of string, int