// Test lambda to functional interface cast
// Functional interface has single abstract method (SAM)

interface Function {
    public function apply(int x): int;
}

interface BiFunction {
    public function apply(int x, int y): int;
}

interface Predicate {
    public function test(int x): bool;
}

interface Consumer {
    public function accept(string s): void;
}

// Test lambda cast to functional interface
Function square = (Function)(int x) => {
    return x * x;
};

print(square.apply(5));
print(square.apply(7));

// Test with BiFunction
BiFunction add = (BiFunction)(int x, int y) => {
    return x + y;
};

print(add.apply(3, 4));
print(add.apply(10, 20));

// Test with Predicate
Predicate isEven = (Predicate)(int x) => {
    return x % 2 == 0;
};

print(isEven.test(4));
print(isEven.test(5));

// Test with Consumer
Consumer printer = (Consumer)(string s) => {
    print("Message: " + s);
};

printer.accept("Hello");
printer.accept("World");

// Test casting lambda result
Function doubler = (Function)(int x) => { return x * 2; };
int result = doubler.apply(21);
print(result);

print("Functional interface casting test passed");
