// Test that HashSet resize preserves all elements
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
    print("Testing HashSet resize preserves elements:");

    HashSet<Key> set = new HashSet<Key>();
    for (int i = 0; i < 30; i++) {
        set.add(new Key(i, i));
    }
    print("size: " + set.size());

    int hits = 0;
    for (int i = 0; i < 30; i++) {
        if (set.contains(new Key(i, i))) {
            hits = hits + 1;
        }
    }
    print("hits: " + hits);

    print("HashSet resize keeps all elements test passed!");
}
main();
