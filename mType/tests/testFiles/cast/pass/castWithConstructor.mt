// Test: Cast with constructor calls
class Parent {
    string name;

    Parent(string n) {
        this.name = n;
        print("Parent constructor: " + n);
    }
}

class Child extends Parent {
    int age;

    Child(string n, int a) : super(n) {
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
