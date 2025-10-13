// Error: Duplicate instance method name in same class

class Calculator {
    int value;

    constructor() {
        this.value = 0;
    }

    function add(int n): void {
        this.value = this.value + n;
    }

    // ERROR: Duplicate instance method
    function add(int x): void {
        this.value = this.value + x + 10;
    }
}

Calculator calc = new Calculator();
print("This should not execute");
