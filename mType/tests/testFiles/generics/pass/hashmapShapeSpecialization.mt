// MYT-273 Phase 6: HashMap<UserClass, V> shape specialization. When K is
// a concrete int-only class with no parent and no user-overridden equals/
// hashCode, the runtime attaches SpecializedCollectionStorage in SHAPE mode.
// put/get/containsKey/remove route through raw int compare instead of
// dispatching to the synth equals/hashCode. This test exercises the
// observable semantics — same contract as the non-specialized fallback.

import * from "../../lib/collections/HashMap.mt";
import * from "../../lib/collections/HashSet.mt";
import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/String.mt";

class Pair {
    private int a;
    private int b;
    public constructor(int a, int b) {
        this.a = a;
        this.b = b;
    }
}

HashMap<Pair, String> m = new HashMap<Pair, String>();
m.put(new Pair(1, 2), new String("a"));
m.put(new Pair(3, 4), new String("b"));
m.put(new Pair(5, 6), new String("c"));

print("size: " + m.size());

Pair p34 = new Pair(3, 4);
print("get 3,4: " + m.get(p34));
print("has 3,4: " + m.containsKey(p34));
print("has 99,99: " + m.containsKey(new Pair(99, 99)));

m.put(new Pair(3, 4), new String("B"));
print("after replace size: " + m.size());
print("get 3,4 again: " + m.get(p34));

m.remove(new Pair(1, 2));
print("after remove size: " + m.size());
print("has 1,2 after remove: " + m.containsKey(new Pair(1, 2)));

HashSet<Pair> s = new HashSet<Pair>();
s.add(new Pair(10, 20));
s.add(new Pair(30, 40));
s.add(new Pair(10, 20));
print("set size: " + s.size());
print("set has 10,20: " + s.contains(new Pair(10, 20)));
print("set has 99,99: " + s.contains(new Pair(99, 99)));
