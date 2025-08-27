// Test enhanced type mismatch exception with location info
class Person {
    string name;
    constructor(name: string) {
        this.name = name;
    }
}

Person p = new Person(42);  // Line 9: Should show type mismatch error with location
print("This should not execute");