// MYT-216: Generic method call with 3 explicit type args drops trailing arg.
//
// EXPECTED:
//   "result: 7"  (apply<Int,Int,Int>(3, 4, sum) returns 7)
//
// ACTUAL (broken):
//   Type error: No matching overload for call to 'Util::apply' with arguments
//   (Int, BinaryFn). Available overloads: 1. Util::apply(T, U, BinaryFn)
//
// The third positional argument disappears from the compiler's view.
// Same call shape with 2 type args works fine.

import * from "../lib/primitives/Int.mt";

interface BinaryFn<T, U, R> {
    function apply(T x, U y): R;
}

class Util {
    public static function <T, U, R> apply(T x, U y, BinaryFn<T, U, R> f): R {
        return f.apply(x, y);
    }
}

BinaryFn<Int, Int, Int> sum = (a, b) -> new Int(a.getValue() + b.getValue());
Int r = Util::apply<Int, Int, Int>(new Int(3), new Int(4), sum);
print("result: " + r.getValue());
