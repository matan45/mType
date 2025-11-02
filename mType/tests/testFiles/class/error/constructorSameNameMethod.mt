// Test: Constructor name cannot be same as a regular method
// Expected: Error - constructor name conflicts with method

class MyClass {
    int value;

    // Constructor
    constructor(int v) {
        this.value = v;
    }

    // This should cause an error - method with same name as constructor
    void constructor() {
        print("This is a method, not a constructor");
    }
}

MyClass obj = new MyClass(10);
