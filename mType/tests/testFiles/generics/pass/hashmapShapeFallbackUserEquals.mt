// Phase 1 limitation: a class that declares its own equals/hashCode is
// NOT shape-spec-eligible (synth-only gate). HashMap<X,V> falls back to
// the generic path; user contract is honored on every dispatch.

import * from "../../lib/collections/HashMap.mt";
import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/String.mt";

class CustomKey {
    private int a;
    private int b;
    public constructor(int a, int b) {
        this.a = a;
        this.b = b;
    }
    public function equals(CustomKey other): bool {
        if (other == null) {
            return false;
        }
        return this.a == other.a && this.b == other.b;
    }
    public function hashCode(): int {
        return this.a * 31 + this.b;
    }
}

HashMap<CustomKey, String> m = new HashMap<CustomKey, String>();
m.put(new CustomKey(1, 2), new String("a"));
m.put(new CustomKey(3, 4), new String("b"));

print("size: " + m.size());
print("get 1,2: " + m.get(new CustomKey(1, 2)));
print("has 3,4: " + m.containsKey(new CustomKey(3, 4)));
print("has 99,99: " + m.containsKey(new CustomKey(99, 99)));

m.remove(new CustomKey(1, 2));
print("after remove size: " + m.size());
print("has 1,2 after remove: " + m.containsKey(new CustomKey(1, 2)));
