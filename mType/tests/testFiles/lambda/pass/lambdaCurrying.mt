// Partial application patterns (currying) test
interface Function<T, R> {
    function apply(T input) : R;
}

interface BiFunction<T, U, R> {
    function apply(T first, U second) : R;
}

class Currier {
    function curry<A, B, C>(BiFunction<A, B, C> func, A first) : Function<B, C> {
        Function<B, C> partial = second -> {
            return func.apply(first, second);
        };
        return partial;
    }
}

print("=== Currying Test ===");

Currier currier = new Currier();

// BiFunction for addition
BiFunction<int, int, int> add = (a, b) -> a + b;

// Curry with first argument = 10
Function<int, int> add10 = currier.curry(add, 10);
print("add10(5): " + add10.apply(5));
print("add10(20): " + add10.apply(20));

// Curry with first argument = 100
Function<int, int> add100 = currier.curry(add, 100);
print("add100(5): " + add100.apply(5));

// BiFunction for multiplication
BiFunction<int, int, int> multiply = (a, b) -> a * b;

Function<int, int> double = currier.curry(multiply, 2);
Function<int, int> triple = currier.curry(multiply, 3);

print("double(5): " + double.apply(5));
print("triple(5): " + triple.apply(5));

// BiFunction with different types
BiFunction<String, int, String> repeat = (s, count) -> {
    String result = "";
    for (int i = 0; i < count; i = i + 1) {
        result = result + s;
    }
    return result;
};

Function<int, String> repeatHi = currier.curry(repeat, "Hi");
print("repeatHi(3): " + repeatHi.apply(3));

Function<int, String> repeatBye = currier.curry(repeat, "Bye");
print("repeatBye(2): " + repeatBye.apply(2));

// Manual currying
Function<int, Function<int, int>> manualCurry = a -> {
    Function<int, int> inner = b -> a + b;
    return inner;
};

Function<int, int> add50 = manualCurry.apply(50);
print("Manually curried add50(25): " + add50.apply(25));

print("Currying complete");
