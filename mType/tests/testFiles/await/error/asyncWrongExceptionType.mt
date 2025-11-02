// Error: Catching wrong exception type

class ExceptionA {
    string msg;
    public constructor(string m) { this.msg = m; }
    public function getMsg(): string { return this.msg; }
}

class ExceptionB {
    string msg;
    public constructor(string m) { this.msg = m; }
    public function getMsg(): string { return this.msg; }
}

class Result {
    int value;
    public constructor(int v) { this.value = v; }
    public function getValue(): int { return this.value; }
}

function async throwsExceptionA(): Promise<Result> {
    print("Throwing ExceptionA");
    throw new ExceptionA("Type A error");
    Result r = new Result(1);
    return r;
}

function async catchWrongType(): Promise<Result> {
    print("Attempting to catch wrong type");

    Result res;
    try {
        res = await throwsExceptionA();
    } catch (ExceptionB eb) {
        // ERROR: Catching ExceptionB but ExceptionA is thrown
        print("Caught ExceptionB");
        res = new Result(100);
    }

    // Exception propagates since wrong type caught
    return res;
}

function async main(): Promise<Result> {
    Result r = await catchWrongType();
    return r;
}

main();
