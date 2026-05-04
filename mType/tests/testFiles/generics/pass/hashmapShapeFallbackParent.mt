// Phase 1 limitation: shape-spec eligibility requires no in-program parent.
// A child class — even when both child and parent are synth-int-only —
// goes through the generic HashMap path. Synthesis still composes via
// super.hashCode/super.equals; correctness is preserved.

import * from "../../lib/collections/HashMap.mt";
import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/String.mt";

class Base {
    private int b;
    public constructor(int b) {
        this.b = b;
    }
}

class Child extends Base {
    private int c;
    public constructor(int b, int c) : super(b) {
        this.c = c;
    }
}

HashMap<Child, String> m = new HashMap<Child, String>();
m.put(new Child(1, 10), new String("alpha"));
m.put(new Child(2, 20), new String("beta"));
m.put(new Child(3, 30), new String("gamma"));

print("size: " + m.size());
print("get 1,10: " + m.get(new Child(1, 10)));
print("has 2,20: " + m.containsKey(new Child(2, 20)));
print("has 9,99: " + m.containsKey(new Child(9, 99)));

m.put(new Child(2, 20), new String("BETA"));
print("after replace size: " + m.size());
print("get 2,20: " + m.get(new Child(2, 20)));

m.remove(new Child(1, 10));
print("after remove size: " + m.size());
print("has 1,10 after remove: " + m.containsKey(new Child(1, 10)));
