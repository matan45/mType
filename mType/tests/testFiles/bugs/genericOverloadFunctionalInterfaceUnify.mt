// MYT-224: Generic overload resolution fails to unify concrete
// functional-interface type with parameterized declared param.
//
// EXPECTED:
//   Util::apply<Int, Int>(new Int(7), doubler) resolves to overload #1
//   (T=Int, R=Int substituted into apply<T,R>(T, UnaryFn<T,R>)). Output: 14
//
// ACTUAL (broken):
//   Type error: No matching overload for call to 'Util::apply' with arguments
//   (Int, UnaryFn). Available overloads:
//     1. Util::apply(T, UnaryFn)
//     2. Util::apply(T, Predicate)
//
// The resolver lists the matching overload `(T, UnaryFn)` in the available
// set yet declines to pick it. Substitution doesn't appear to run before the
// match check.

import * from "../lib/functional/Predicate.mt";
import * from "../lib/primitives/Int.mt";

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
