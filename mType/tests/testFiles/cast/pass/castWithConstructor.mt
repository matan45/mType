// Test: Cast with constructor calls
class Parent {
    public string name;

    public constructor(string n) {
        this.name = n;
        print("Parent constructor: " + n);
    }
}

class Child extends Parent {
    public int age;

    public constructor(string n, int a) : super(n) {
        this.age = a;
        print("Child constructor: " + (string)a);
    }
}

Child child = new Child("Alice", 10);
Parent parent = (Parent)child;
print(parent.name);  // Expected: Alice

// Expected output:
// Parent constructor: Alice
// Child constructor: 10
// Alice
