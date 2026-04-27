// Nested lambda currying test - manual curry returning a Function<Int,Int>
import * from "../../lib/primitives/Int.mt";

interface Function<T, R> {
    function apply(T input): R;
}

print("=== Nested Lambda Curry Test ===");

Function<Int, Function<Int, Int>> add = x -> (y -> new Int(x.getValue() + y.getValue()));

Function<Int, Int> add5 = add.apply(new Int(5));
Function<Int, Int> add10 = add.apply(new Int(10));

print("add5(3) = " + add5.apply(new Int(3)).getValue());
print("add5(7) = " + add5.apply(new Int(7)).getValue());
print("add10(2) = " + add10.apply(new Int(2)).getValue());
print("add(1)(1) = " + add.apply(new Int(1)).apply(new Int(1)).getValue());
