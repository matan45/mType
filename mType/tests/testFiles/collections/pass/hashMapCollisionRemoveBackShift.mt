// Test HashMap remove with three colliding keys triggers correct back-shift
// Keys hash to slot 0; probed to slots 0, 1, 2. After removing the first,
// the other two must remain reachable (back-shift restores probe sequence).
import * from "../../lib/collections/HashMap.mt";
import * from "../../lib/primitives/String.mt";

class Key {
    int h;
    int id;
    public constructor(int hash, int identifier) {
        this.h = hash;
        this.id = identifier;
    }
    public function hashCode(): int { return this.h; }
    public function equals(Key other): bool { return this.id == other.id; }
}

function main(): void {
    print("Testing HashMap collision remove back-shift:");

    HashMap<Key, String> map = new HashMap<Key, String>();
    Key k1 = new Key(0, 1);
    Key k2 = new Key(0, 2);
    Key k3 = new Key(0, 3);

    map.put(k1, new String("A"));
    map.put(k2, new String("B"));
    map.put(k3, new String("C"));
    print("size before: " + map.size());

    map.remove(k1);
    print("size after remove: " + map.size());

    print("contains k2: " + map.containsKey(k2));
    print("get k2: " + map.get(k2));
    print("contains k3: " + map.containsKey(k3));
    print("get k3: " + map.get(k3));

    print("HashMap collision remove back-shift test passed!");
}
main();
