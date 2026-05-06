// Test that put with an existing key overwrites the value (not duplicate)
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
    print("Testing put overwrite same key:");

    HashMap<Key, String> map = new HashMap<Key, String>();
    Key k = new Key(7, 1);
    map.put(k, new String("a"));
    map.put(k, new String("b"));

    print("size: " + map.size());
    print("get: " + map.get(k));

    print("HashMap put-overwrite test passed!");
}
main();
