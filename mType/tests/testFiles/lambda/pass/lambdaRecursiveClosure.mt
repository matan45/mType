// Lambda with recursive calls test
interface Function {
    function apply(int x) : int;
}

class RecursiveHolder {
    Function func;

    function setFunc(Function f) {
        this.func = f;
    }

    function call(int x) : int {
        return this.func.apply(x);
    }
}

print("=== Recursive Closure Test ===");

// Factorial using recursive lambda
RecursiveHolder factorial = new RecursiveHolder();
factorial.setFunc(n -> {
    if (n <= 1) {
        return 1;
    } else {
        return n * factorial.call(n - 1);
    }
});

print("Factorial(5) = " + factorial.call(5));
print("Factorial(6) = " + factorial.call(6));

// Fibonacci using recursive lambda
RecursiveHolder fib = new RecursiveHolder();
fib.setFunc(n -> {
    if (n <= 1) {
        return n;
    } else {
        return fib.call(n - 1) + fib.call(n - 2);
    }
});

print("Fib(7) = " + fib.call(7));
print("Fib(10) = " + fib.call(10));

print("Recursive closure complete");
