// Test: Lambda with constraint-bound interface
// @Script

interface Processor<T> {
    process(input: T) : T;
    validate(input: T) : Bool;
}

class NumberProcessor : Processor<Int> {
    private transform: (Int) -> Int;
    private validator: (Int) -> Bool;

    constructor(t: (Int) -> Int, v: (Int) -> Bool) {
        this.transform = t;
        this.validator = v;
    }

    process(input: Int) : Int {
        if (this.validate(input)) {
            return this.transform(input);
        }
        return input;
    }

    validate(input: Int) : Bool {
        return this.validator(input);
    }
}

class StringProcessor : Processor<String> {
    private transform: (String) -> String;
    private validator: (String) -> Bool;

    constructor(t: (String) -> String, v: (String) -> Bool) {
        this.transform = t;
        this.validator = v;
    }

    process(input: String) : String {
        if (this.validate(input)) {
            return this.transform(input);
        }
        return input;
    }

    validate(input: String) : Bool {
        return this.validator(input);
    }
}

main() : Void {
    // Number processor with lambda constraints
    let numProc: Processor<Int> = new NumberProcessor(
        (n: Int) : Int => { return n * 2; },
        (n: Int) : Bool => { return n > 0; }
    );

    let result1 = numProc.process(5);
    print("Processed positive: " + result1.toString());
    assert(result1 == 10, "Should double positive number");

    let result2 = numProc.process(-5);
    print("Processed negative: " + result2.toString());
    assert(result2 == -5, "Should not process negative number");

    // String processor with lambda constraints
    let strProc: Processor<String> = new StringProcessor(
        (s: String) : String => { return s + "!"; },
        (s: String) : Bool => { return s.length() > 0; }
    );

    let result3 = strProc.process("Hello");
    print("Processed: " + result3);
    assert(result3 == "Hello!", "Should add exclamation");

    print("Constrained interface test passed");
}
