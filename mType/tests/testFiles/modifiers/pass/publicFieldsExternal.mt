// Test: Public fields accessible from external classes
class Person {
    public string name;
    public int age;

    constructor(string n, int a) {
        name = n;
        age = a;
    }
}

// External access to public fields
Person p = new Person("Alice", 30);
print(p.name);  // Expected: Alice
print(p.age);   // Expected: 30

// Modify public fields externally
p.name = "Bob";
p.age = 25;
print(p.name);  // Expected: Bob
print(p.age);   // Expected: 25
