// Test: `obj isClassOf T` inside a method on a generic class resolves T
// via the receiver's generic bindings. Container<Int>::check(x) should
// return true when x is an Int instance and false otherwise.
//
// This is the headline test for MYT-41 requirement 2.

import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/String.mt";

class Container<T> {
    T value;

    public constructor(T v) {
        this.value = v;
    }

    public function isMyT(Object o): bool {
        return o isClassOf T;
    }
}

Container<Int> intBox = new Container<Int>(new Int(1));
Container<String> strBox = new Container<String>(new String("hi"));

Int one = new Int(1);
String hello = new String("hello");

// intBox's T resolves to Int at runtime
print(intBox.isMyT(one));    // true
print(intBox.isMyT(hello));  // false

// strBox's T resolves to String at runtime
print(strBox.isMyT(one));    // false
print(strBox.isMyT(hello));  // true
