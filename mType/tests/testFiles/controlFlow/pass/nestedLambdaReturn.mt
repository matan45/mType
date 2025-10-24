// Test return through nested lambda calls

interface Function<T, R> {
    function apply(T input): R;
}

interface Supplier<T> {
    function get(): T;
}

function processWithLambda(int value, Function<int, string> func): string {
    return func.apply(value);
}

function main(): void {
    print("Testing nested lambda return");

    // Simple lambda return
    Function<int, string> func1 = (int x) -> {
        if (x > 10) {
            return "Large";
        }
        return "Small";
    };

    print("func1(5): " + func1.apply(5));
    print("func1(15): " + func1.apply(15));

    // Nested lambda usage
    string result = processWithLambda(20, (int x) -> {
        if (x > 10) {
            return "Value is large: " + x;
        }
        return "Value is small: " + x;
    });

    print("Nested result: " + result);

    // Lambda returning lambda
    Supplier<Function<int, string>> factory = () -> {
        return (int x) -> {
            return "Nested: " + x;
        };
    };

    Function<int, string> created = factory.get();
    print("Factory result: " + created.apply(42));

    print("Nested lambda return test completed");
}

main();
