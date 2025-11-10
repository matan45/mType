import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/String.mt";

// Partial application patterns (currying) test
interface Function<T, R> {
    function apply(T input) : R;
}

interface BiFunction<T, U, R> {
    function apply(T first, U second) : R;
}

class Currier {
    public function <A, B, C> curry(BiFunction<A, B, C> func, A first) : Function<B, C> {
        Function<B, C> partial = second -> {
            return func.apply(first, second);
        };
        return partial;
    }
}

print("=== Currying Test ===");

Currier currier = new Currier();

// BiFunction for addition
BiFunction<Int, Int, Int> add = (a, b) -> a + b;

// Curry with first argument = 10
Function<Int, Int> add10 = currier.curry<Int, Int, Int>(add, 10);
print("add10(5): " + add10.apply(5));
print("add10(20): " + add10.apply(20));

// Curry with first argument = 100
Function<Int, Int> add100 = currier.curry<Int, Int, Int>(add, 100);
print("add100(5): " + add100.apply(5));

// BiFunction for multiplication
BiFunction<Int, Int, Int> multiply = (a, b) -> a * b;

Function<Int, Int> double = currier.curry<Int, Int, Int>(multiply, 2);
Function<Int, Int> triple = currier.curry<Int, Int, Int>(multiply, 3);

print("double(5): " + double.apply(5));
print("triple(5): " + triple.apply(5));

// BiFunction with different types
BiFunction<String, Int, String> repeat = (s, count) -> {
    String result = "";
    for (int i = 0; i < count.getValue(); i = i + 1) {
        result = result + s;
    }
    return result;
};

Function<Int, String> repeatHi = currier.curry<String, Int, String>(repeat, "Hi");
print("repeatHi(3): " + repeatHi.apply(3));

Function<Int, String> repeatBye = currier.curry<String, Int, String>(repeat, "Bye");
print("repeatBye(2): " + repeatBye.apply(2));

// Manual currying
Function<Int, Function<Int, Int>> manualCurry = a -> {
    Function<Int, Int> inner = b -> a + b;
    return inner;
};

Function<Int, Int> add50 = manualCurry.apply(50);
print("Manually curried add50(25): " + add50.apply(25));

print("Currying complete");
