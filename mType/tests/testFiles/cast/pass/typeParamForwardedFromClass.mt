// MYT-228: forwarding a CLASS-level T into a method-level U. Inside an
// instance method on Box<T>, calling Helper::probe<T>(o) must resolve T
// against the receiver's class-level bindings (genericTypeBindings on
// `this`), not against any caller frame. The valueKind=1 forwarder's
// fallback path covers this by walking getThisInstanceRaw bindings
// when the per-frame map doesn't contain the name.

import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/String.mt";

class Helper {
    public static function <U> probe(Object o): bool {
        return o isClassOf U;
    }
}

class Box<T> {
    T value;
    public constructor(T v) { this.value = v; }

    // T here is class-level; forwarded into Helper.probe<U>'s frame.
    public function checkAgainstT(Object o): bool {
        return Helper::probe<T>(o);
    }
}

Box<Int> intBox = new Box<Int>(new Int(1));
Box<String> strBox = new Box<String>(new String("hi"));

Int n = new Int(42);
String s = new String("world");

print(intBox.checkAgainstT(n));   // true  — Box<Int>::T = Int
print(intBox.checkAgainstT(s));   // false
print(strBox.checkAgainstT(n));   // false
print(strBox.checkAgainstT(s));   // true  — Box<String>::T = String
