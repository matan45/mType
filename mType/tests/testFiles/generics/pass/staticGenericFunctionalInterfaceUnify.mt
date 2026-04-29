// Generic static-method overload resolution must substitute explicit type
// arguments into a parameterized declared parameter (e.g. UnaryFn<T,R>)
// before unifying it with the concrete argument type (UnaryFn<Int,Int>).
// MYT-224 regression.

import * from "../../lib/functional/Predicate.mt";
import * from "../../lib/primitives/Int.mt";

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

UnaryFn<Int, Int> doubler = x -> new Int(x.getValue() * 2);
Int r = Util::apply<Int, Int>(new Int(7), doubler);
print("doubler(7) = " + r.getValue());
