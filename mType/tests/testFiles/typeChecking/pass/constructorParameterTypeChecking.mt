class Person {
    public string name;
    public int age;
    public bool active;

    constructor(string n, int a, bool act) {
        name = n;
        age = a;
        active = act;
    }
}

// Test correct constructor call
Person p1 = new Person("Alice", 25, true);
print(p1.name + " is " + p1.age);