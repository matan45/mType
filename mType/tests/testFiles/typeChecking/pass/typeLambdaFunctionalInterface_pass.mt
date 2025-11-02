// Test functional interface Single Abstract Method (SAM) type verification
interface Runnable {
    function run() : void;
}

interface Supplier {
    function supply() : int;
}

interface Consumer {
    function consume(string message) : void;
}

interface BiFunction {
    function apply(int a, int b) : int;
}

print("=== Functional Interface SAM Verification Test ===");

// SAM with no parameters, void return
Runnable task = () -> {
    print("Task executed");
};
task.run();

// SAM with no parameters, int return
Supplier numberSupplier = () -> 42;
print("Supplied value: " + numberSupplier.supply());

// SAM with one parameter, void return
Consumer logger = msg -> {
    print("Log: " + msg);
};
logger.consume("Test message");

// SAM with two parameters, int return
BiFunction adder = (x, y) -> x + y;
print("5 + 3 = " + adder.apply(5, 3));

BiFunction multiplier = (a, b) -> a * b;
print("4 * 7 = " + multiplier.apply(4, 7));

// Verify each lambda conforms to its SAM type
interface Predicate {
    function test(int value) : bool;
}

Predicate isEven = n -> n % 2 == 0;
Predicate isPositive = n -> n > 0;

print("10 is even: " + isEven.test(10));
print("10 is positive: " + isPositive.test(10));
print("-5 is even: " + isEven.test(-5));
print("-5 is positive: " + isPositive.test(-5));

print("SAM verification successful");
