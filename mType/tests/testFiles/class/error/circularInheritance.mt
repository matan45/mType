// Test: Circular inheritance - A extends B, B extends A
// Expected: Error - circular inheritance detected

class A extends B {
    int value;

    constructor(int value) {
        super(value);
        this.value = value;
    }
}

class B extends A {
    int otherValue;

    constructor(int value) {
        super(value);
        this.otherValue = value;
    }
}

// This should never execute due to circular inheritance error
A obj = new A(10);
