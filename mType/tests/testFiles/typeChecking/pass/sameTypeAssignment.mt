class Person {
    public string name;
    constructor(string n) {
        name = n;
    }
}

Person p1 = new Person("Alice");
Person p2 = new Person("Bob");
p2 = p1;  // Should work - same type

// Verify assignment worked by checking the name
print(p2.name);  // Should print "Alice"