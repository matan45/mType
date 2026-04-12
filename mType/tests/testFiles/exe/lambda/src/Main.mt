// Test: Lambda & Closures in standalone exe

interface IntFunction {
    function apply(int x): int;
}

interface StringFunction {
    function apply(string s): string;
}

interface IntPredicate {
    function test(int x): bool;
}

function applyTwice(IntFunction fn, int value): int {
    return fn.apply(fn.apply(value));
}

@EntryPoint
class App {
    public static function main(string[] args): void {
        // Expression lambda
        IntFunction doubler = x -> x * 2;
        print("Double 5: " + doubler.apply(5));

        // Block lambda
        IntFunction factorial = n -> {
            int result = 1;
            for (int i = 1; i <= n; i = i + 1) {
                result = result * i;
            }
            return result;
        };
        print("Factorial 5: " + factorial.apply(5));

        // Lambda as callback parameter (higher-order)
        int result = applyTwice(doubler, 3);
        print("Double twice 3: " + result);

        // Closure capturing local variable
        int offset = 10;
        IntFunction adder = x -> x + offset;
        print("Add offset to 5: " + adder.apply(5));

        // String lambda
        StringFunction exclaim = s -> s + "!";
        print("Exclaim: " + exclaim.apply("hello"));

        // Predicate lambda
        IntPredicate isEven = x -> x % 2 == 0;
        print("4 is even: " + isEven.test(4));
        print("7 is even: " + isEven.test(7));

        print("Lambda test passed");
    }
}
