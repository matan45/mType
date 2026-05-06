// Test removing the MIDDLE of a 3-key collision chain via back-shift
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
    print("Testing HashMap remove middle of collision chain:");

    HashMap<Key, String> map = new HashMap<Key, String>();
    Key k1 = new Key(0, 1);
    Key k2 = new Key(0, 2);
    Key k3 = new Key(0, 3);

    map.put(k1, new String("A"));
    map.put(k2, new String("B"));
    map.put(k3, new String("C"));

    map.remove(k2);
    print("size: " + map.size());
    print("contains k1: " + map.containsKey(k1));
    print("get k1: " + map.get(k1));
    print("contains k3: " + map.containsKey(k3));
    print("get k3: " + map.get(k3));

    print("HashMap remove middle test passed!");
}
main();
