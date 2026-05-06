// Test that get() on a removed colliding key returns null, then NPE on access
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

HashMap<Key, String> map = new HashMap<Key, String>();
Key k1 = new Key(0, 1);
Key k2 = new Key(0, 2);
Key k3 = new Key(0, 3);
map.put(k1, new String("A"));
map.put(k2, new String("B"));
map.put(k3, new String("C"));
map.remove(k1);

String missing = map.get(k1);
int len = missing.length();
print("should not reach: " + len);
