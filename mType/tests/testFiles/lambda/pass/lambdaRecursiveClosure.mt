// Lambda with recursive calls test
import * from "../../lib/primitives/Int.mt";

interface Function<T, R> {
    function apply(T input) : R;
}

class RecursiveHolder {
    Function<Int, Int> func;

    public function setFunc(Function<Int, Int> f) {
        this.func = f;
    }

    public function call(Int x) : Int {
        return this.func.apply(x);
    }
}

print("=== Recursive Closure Test ===");

// Factorial using recursive lambda
RecursiveHolder factorial = new RecursiveHolder();
factorial.setFunc(n -> {
    if (n.getValue() <= 1) {
        return new Int(1);
    } else {
        return new Int(n.getValue() * factorial.call(new Int(n.getValue() - 1)).getValue());
    }
});

Int five = new Int(5);
Int six = new Int(6);
print("Factorial(5) = " + factorial.call(five).getValue());
print("Factorial(6) = " + factorial.call(six).getValue());

// Fibonacci using recursive lambda
RecursiveHolder fib = new RecursiveHolder();
fib.setFunc(n -> {
    if (n.getValue() <= 1) {
        return n;
    } else {
        Int n1 = new Int(n.getValue() - 1);
        Int n2 = new Int(n.getValue() - 2);
        return new Int(fib.call(n1).getValue() + fib.call(n2).getValue());
    }
});

Int seven = new Int(7);
Int ten = new Int(10);
print("Fib(7) = " + fib.call(seven).getValue());
print("Fib(10) = " + fib.call(ten).getValue());

print("Recursive closure complete");
