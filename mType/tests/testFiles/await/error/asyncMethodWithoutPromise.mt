// Error: Async method without Promise return type

class Result {
    int value;

    constructor(int v) {
        this.value = v;
    }
}

class Calculator {
    int base;

    constructor(int b) {
        this.base = b;
    }

    // ERROR: Async method must return Promise<T>
    function async calculate(int x): Result {
        int sum = this.base + x;
        Result r = new Result(sum);
        return r;
    }
}

print("This should fail");
