// Test wrap-around collision: keys hash to last slot, probe wraps to slot 0 then 1
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
    print("Testing wrap-around collision back-shift:");

    HashMap<Key, String> map = new HashMap<Key, String>();
    Key k1 = new Key(31, 1);
    Key k2 = new Key(31, 2);
    Key k3 = new Key(31, 3);

    map.put(k1, new String("A"));
    map.put(k2, new String("B"));
    map.put(k3, new String("C"));

    print("get k1: " + map.get(k1));
    print("get k2: " + map.get(k2));
    print("get k3: " + map.get(k3));

    map.remove(k1);
    print("after remove k1, size: " + map.size());
    print("contains k2: " + map.containsKey(k2));
    print("get k2: " + map.get(k2));
    print("contains k3: " + map.containsKey(k3));
    print("get k3: " + map.get(k3));

    print("HashMap wrap-around collision test passed!");
}
main();
