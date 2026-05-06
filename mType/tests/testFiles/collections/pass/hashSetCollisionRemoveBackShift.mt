// Test HashSet remove with three colliding elements triggers correct back-shift
import * from "../../lib/collections/HashSet.mt";

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
    print("Testing HashSet collision remove back-shift:");

    HashSet<Key> set = new HashSet<Key>();
    Key k1 = new Key(0, 1);
    Key k2 = new Key(0, 2);
    Key k3 = new Key(0, 3);

    set.add(k1);
    set.add(k2);
    set.add(k3);
    print("size before: " + set.size());

    set.remove(k1);
    print("size after remove: " + set.size());
    print("contains k2: " + set.contains(k2));
    print("contains k3: " + set.contains(k3));
    print("contains k1: " + set.contains(k1));

    print("HashSet collision remove back-shift test passed!");
}
main();
