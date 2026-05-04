// Phase 1 limitation: shape-spec inline-slot budget caps at 4 INT fields.
// A class with 5+ ints is still synth-eligible (Phase 1 of MYT-274), but
// the storage doesn't attach. HashMap goes through the generic path with
// the synthesized equals/hashCode. Correctness verified end-to-end.

import * from "../../lib/collections/HashMap.mt";
import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/String.mt";

class FiveFields {
    private int a;
    private int b;
    private int c;
    private int d;
    private int e;
    public constructor(int a, int b, int c, int d, int e) {
        this.a = a;
        this.b = b;
        this.c = c;
        this.d = d;
        this.e = e;
    }
}

HashMap<FiveFields, String> m = new HashMap<FiveFields, String>();
m.put(new FiveFields(1, 2, 3, 4, 5), new String("first"));
m.put(new FiveFields(6, 7, 8, 9, 10), new String("second"));

print("size: " + m.size());
print("get first: " + m.get(new FiveFields(1, 2, 3, 4, 5)));
print("has second: " + m.containsKey(new FiveFields(6, 7, 8, 9, 10)));
print("has missing: " + m.containsKey(new FiveFields(0, 0, 0, 0, 0)));

m.remove(new FiveFields(1, 2, 3, 4, 5));
print("after remove size: " + m.size());
print("has first after remove: " + m.containsKey(new FiveFields(1, 2, 3, 4, 5)));
