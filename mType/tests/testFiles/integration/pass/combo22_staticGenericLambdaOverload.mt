// Combo 22: Static + Generic + Lambda + Overloading
// Tests: Util::apply overloaded by functional-interface type at same arity.
// Same-arity overloads dodge mType's overload-resolution edge cases with
// 3-type-parameter generic methods. UnaryFn vs Predicate dispatch.

import * from "../../lib/functional/Predicate.mt";
import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/String.mt";

interface UnaryFn<T, R> {
    function apply(T x): R;
}

class Util {
    public static function <T, R> apply(T x, UnaryFn<T, R> f): R {
        return f.apply(x);
    }

    public static function <T> apply(T x, Predicate<T> p): bool {
        return p.test(x);
    }
}

function main(): void {
    print("=== Combo 22: Static + Generic + Lambda + Overload ===");

    print("--- Apply with UnaryFn ---");
    UnaryFn<Int, Int> doubler = x -> new Int(x.getValue() * 2);
    Int r1 = Util::apply<Int, Int>(new Int(7), doubler);
    print("doubler(7) = " + r1.getValue());

    UnaryFn<String, Int> length = s -> new Int(s.length());
    Int r2 = Util::apply<String, Int>(new String("hello"), length);
    print("length(hello) = " + r2.getValue());

    print("--- Apply with Predicate ---");
    Predicate<Int> isEven = n -> n.getValue() % 2 == 0;
    bool r3 = Util::apply<Int>(new Int(8), isEven);
    print("isEven(8) = " + r3);
    bool r4 = Util::apply<Int>(new Int(7), isEven);
    print("isEven(7) = " + r4);

    Predicate<String> isLong = s -> s.length() > 4;
    bool r5 = Util::apply<String>(new String("hi"), isLong);
    print("isLong(hi) = " + r5);
    bool r6 = Util::apply<String>(new String("hello"), isLong);
    print("isLong(hello) = " + r6);

    print("=== Combo 22 Complete ===");
}

main();
