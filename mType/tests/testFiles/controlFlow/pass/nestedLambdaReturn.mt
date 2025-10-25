// Test return through nested lambda calls
import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/String.mt";

interface Function<T, R> {
    function apply(T input): R;
}

interface Supplier<T> {
    function get(): T;
}

function processWithLambda(Int value, Function<Int, String> func): String {
    return func.apply(value);
}

function main(): void {
    print("Testing nested lambda return");

    // Simple lambda return
    Function<Int, String> func1 = x -> {
        if (x.value > 10) {
            return new String("Large");
        }
        return new String("Small");
    };

    print("func1(5): " + func1.apply(new Int(5)).toString());
    print("func1(15): " + func1.apply(new Int(15)).toString());

    // Nested lambda usage
    String result = processWithLambda(new Int(20), x -> {
        if (x.value > 10) {
            return new String("Value is large: " + x.value);
        }
        return new String("Value is small: " + x.value);
    });

    print("Nested result: " + result.toString());

    // Lambda returning lambda
    Supplier<Function<Int, String>> factory = () -> {
        return x -> {
            return new String("Nested: " + x.value);
        };
    };

    Function<Int, String> created = factory.get();
    print("Factory result: " + created.apply(new Int(42)).toString());

    print("Nested lambda return test completed");
}

main();
