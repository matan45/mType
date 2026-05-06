// Test that a slot freed by remove can be re-used by a subsequent put with same key
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
    print("Testing remove then put same key:");

    HashMap<Key, String> map = new HashMap<Key, String>();
    Key k = new Key(0, 1);
    map.put(k, new String("x"));
    map.remove(k);
    map.put(k, new String("y"));

    print("size: " + map.size());
    print("get: " + map.get(k));

    print("HashMap remove-then-put test passed!");
}
main();
