// Test that triggering a resize preserves all keys (rehash with new mask)
// Default capacity = 32, threshold = 24. Adding 30 keys forces a resize at 25.
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
    print("Testing HashMap resize preserves keys:");

    HashMap<Key, String> map = new HashMap<Key, String>();
    for (int i = 0; i < 30; i++) {
        map.put(new Key(i, i), new String("v" + i));
    }
    print("size: " + map.size());

    int hits = 0;
    for (int i = 0; i < 30; i++) {
        Key probe = new Key(i, i);
        if (map.containsKey(probe)) {
            hits = hits + 1;
        }
    }
    print("hits: " + hits);

    print("HashMap resize keeps all keys test passed!");
}
main();
