// Test: 5000 puts with heavily colliding hashes (hash = i % 16) across many
// resizes; every key must remain reachable and removals must not strand
// colliding neighbours.
import * from "../../lib/collections/HashMap.mt";
import * from "../../lib/primitives/Int.mt";

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
    print("Testing HashMap 5000 colliding puts:");

    HashMap<Key, Int> map = new HashMap<Key, Int>();
    for (int i = 0; i < 5000; i++) {
        map.put(new Key(i % 16, i), new Int(i * 2));
    }
    print("size: " + map.size());

    int hits = 0;
    for (int i = 0; i < 5000; i++) {
        Int v = map.get(new Key(i % 16, i));
        if (v != null && v.getValue() == i * 2) {
            hits = hits + 1;
        }
    }
    print("hits: " + hits);

    // Remove every third key, then verify the survivors are still reachable
    // through their collision chains.
    for (int i = 0; i < 5000; i = i + 3) {
        map.remove(new Key(i % 16, i));
    }
    print("size after removes: " + map.size());

    int survivors = 0;
    for (int i = 0; i < 5000; i++) {
        if (i % 3 != 0 && map.containsKey(new Key(i % 16, i))) {
            survivors = survivors + 1;
        }
    }
    print("survivors: " + survivors);

    print("HashMap many colliding puts test passed!");
}
main();
