// Test: Lambda with constraint-bound interface
// @Script

interface Processor<T> {
    function process(input: T) : T;
    function validate(input: T) : Bool;
}

class NumberProcessor implements Processor<Int> {
    private (Int) -> Int transform;
    private (Int) -> Bool validator;

    constructor(t: (Int) -> Int, v: (Int) -> Bool) {
        this.transform = t;
        this.validator = v;
    }

    public function process(input: Int) : Int {
        if (this.validate(input)) {
            return this.transform(input);
        }
        return input;
    }

    public function validate(input: Int) : Bool {
        return this.validator(input);
    }
}

class StringProcessor implements Processor<String> {
    private (String) -> String transform;
    private (String) -> Bool validator;

    constructor(t: (String) -> String, v: (String) -> Bool) {
        this.transform = t;
        this.validator = v;
    }

    public function process(input: String) : String {
        if (this.validate(input)) {
            return this.transform(input);
        }
        return input;
    }

    public function validate(input: String) : Bool {
        return this.validator(input);
    }
}

function main() : void {
    // Number processor with lambda constraints
    Processor<Int> numProc = new NumberProcessor(
        (n: Int) : Int => { return n * 2; },
        (n: Int) : Bool => { return n > 0; }
    );

    Int result1 = numProc.process(5);
    print("Processed positive: " + result1.toString());
    assert(result1 == 10, "Should double positive number");

    Int result2 = numProc.process(-5);
    print("Processed negative: " + result2.toString());
    assert(result2 == -5, "Should not process negative number");

    // String processor with lambda constraints
    Processor<String> strProc = new StringProcessor(
        (s: String) : String => { return s + "!"; },
        (s: String) : Bool => { return s.length() > 0; }
    );

    String result3 = strProc.process("Hello");
    print("Processed: " + result3);
    assert(result3 == "Hello!", "Should add exclamation");

    print("Constrained interface test passed");
}
