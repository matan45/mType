// MYT-274: user class with synthesized hashCode/equals can be used as a
// HashMap key. Put/get/containsKey/remove all see consistent identity
// regardless of which instance object is presented to lookup.

import * from "../../lib/collections/HashMap.mt";
import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/String.mt";

class Coord {
    private int x;
    private int y;
    public constructor(int x, int y) {
        this.x = x;
        this.y = y;
    }
}

HashMap<Coord, String> map = new HashMap<Coord, String>();
map.put(new Coord(1, 2), new String("origin-ish"));
map.put(new Coord(5, 7), new String("far"));

Coord lookup12 = new Coord(1, 2);
Coord lookup99 = new Coord(99, 99);

print("size: " + map.size());
print("get 1,2: " + map.get(lookup12));
print("contains 1,2: " + map.containsKey(lookup12));
print("contains 99,99: " + map.containsKey(lookup99));

map.remove(lookup12);
print("after remove size: " + map.size());
print("contains 1,2 after remove: " + map.containsKey(lookup12));
