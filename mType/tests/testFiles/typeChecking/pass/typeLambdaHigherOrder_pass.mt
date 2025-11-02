// Test higher-order function type validation (function returning function)
interface UnaryIntOp {
    public function apply(int x) : int;
}

interface UnaryOpFactory {
    public function create(int factor) : UnaryIntOp;
}

interface BinaryIntOp {
    public function apply(int a, int b) : int;
}

interface Composer {
    public function compose(UnaryIntOp f, UnaryIntOp g) : UnaryIntOp;
}

print("=== Higher-Order Function Type Validation Test ===");

// Function factory - returns a lambda with proper type
UnaryOpFactory multiplierFactory = factor -> {
    return x -> x * factor;
};

UnaryIntOp times2 = multiplierFactory.create(2);
UnaryIntOp times5 = multiplierFactory.create(5);

print("10 * 2 = " + times2.apply(10));
print("10 * 5 = " + times5.apply(10));

// Function composition - takes functions, returns function
Composer compositor = (f, g) -> {
    return x -> f.apply(g.apply(x));
};

UnaryIntOp addTen = x -> x + 10;
UnaryIntOp double = x -> x * 2;

// Compose: first add 10, then double
UnaryIntOp addThenDouble = compositor.compose(double, addTen);
print("(5 + 10) * 2 = " + addThenDouble.apply(5));

// Compose: first double, then add 10
UnaryIntOp doubleThenAdd = compositor.compose(addTen, double);
print("(5 * 2) + 10 = " + doubleThenAdd.apply(5));

// Currying - function returning function
interface Curried {
    public function curry(int x) : UnaryIntOp;
}

Curried adder = a -> {
    return b -> a + b;
};

UnaryIntOp add5 = adder.curry(5);
UnaryIntOp add100 = adder.curry(100);

print("10 + 5 = " + add5.apply(10));
print("10 + 100 = " + add100.apply(10));

print("Higher-order function types validated");
