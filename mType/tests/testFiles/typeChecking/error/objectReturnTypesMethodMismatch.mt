class Person {
    string name;
    
    constructor(string n) {
        name = n;
    }
    
    function getName(): string {
        return name;
    }
}

Person p = new Person("Alice");
int wrongName = p.getName();  // Should fail