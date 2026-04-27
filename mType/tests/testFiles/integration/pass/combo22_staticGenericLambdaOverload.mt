// Combo 22: Static + Generic + Lambda + Overloading
// Tests: Util::apply<T,R> vs Util::apply<T,U,R> dispatch by arity

import * from "../../lib/functional/Function.mt";
import * from "../../lib/functional/BiFunction.mt";
import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/String.mt";

class Util {
    public static function <T, R> apply(T x, Function<T, R> f): R {
        return f.apply(x);
    }

    public static function <T, U, R> apply(T x, U y, BiFunction<T, U, R> f): R {
        return f.apply(x, y);
    }
}

function main(): void {
    print("=== Combo 22: Static + Generic + Lambda + Overload ===");

    print("--- Unary apply ---");
    Function<Int, Int> doubler = x -> new Int(x.getValue() * 2);
    Int r1 = Util::apply<Int, Int>(new Int(7), doubler);
    print("doubler(7) = " + r1.getValue());

    Function<String, Int> length = s -> new Int(s.length());
    Int r2 = Util::apply<String, Int>(new String("hello"), length);
    print("length(hello) = " + r2.getValue());

    print("--- Binary apply ---");
    BiFunction<Int, Int, Int> sum = (a, b) -> new Int(a.getValue() + b.getValue());
    Int r3 = Util::apply<Int, Int, Int>(new Int(3), new Int(4), sum);
    print("sum(3, 4) = " + r3.getValue());

    BiFunction<String, String, String> concat = (a, b) -> new String(a.getValue() + "-" + b.getValue());
    String r4 = Util::apply<String, String, String>(new String("foo"), new String("bar"), concat);
    print("concat(foo, bar) = " + r4.getValue());

    print("--- Mixed type binary ---");
    BiFunction<String, Int, String> repeat = (s, n) -> {
        string out = "";
        for (int i = 0; i < n.getValue(); i++) {
            out = out + s.getValue();
        }
        return new String(out);
    };
    String r5 = Util::apply<String, Int, String>(new String("ab"), new Int(3), repeat);
    print("repeat(ab, 3) = " + r5.getValue());

    print("=== Combo 22 Complete ===");
}

main();
