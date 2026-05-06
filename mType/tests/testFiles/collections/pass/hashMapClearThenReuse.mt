// Test that clear() resets state so subsequent puts work correctly
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
    print("Testing HashMap clear then reuse:");

    HashMap<Key, String> map = new HashMap<Key, String>();
    for (int i = 0; i < 5; i++) {
        map.put(new Key(i, i), new String("first" + i));
    }
    print("before clear: " + map.size());

    map.clear();
    print("after clear: " + map.size());

    for (int i = 100; i < 105; i++) {
        map.put(new Key(i, i), new String("second" + i));
    }
    print("after reuse: " + map.size());

    int hits = 0;
    for (int i = 100; i < 105; i++) {
        if (map.containsKey(new Key(i, i))) {
            hits = hits + 1;
        }
    }
    print("reuse hits: " + hits);

    int oldHits = 0;
    for (int i = 0; i < 5; i++) {
        if (map.containsKey(new Key(i, i))) {
            oldHits = oldHits + 1;
        }
    }
    print("old hits: " + oldHits);

    print("HashMap clear then reuse test passed!");
}
main();
