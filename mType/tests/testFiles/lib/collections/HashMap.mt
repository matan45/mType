// HashMap<K,V> - Open-addressing hash table with linear probing on flat 1D arrays
import * from "../../lib/interfaces/Map.mt";
import * from "../../lib/interfaces/MapEntry.mt";
import * from "../../lib/Iterator.mt";
import * from "../../lib/iterators/HashMapKeyIterator.mt";
import * from "../../lib/iterators/HashMapEntryIterator.mt";
import * from "../../lib/iterators/HashMapValueIterator.mt";
import * from "../../lib/stream/Stream.mt";
import * from "../../lib/stream/StreamImpl.mt";

// Storage layout: 4 parallel flat arrays of length `capacity` (power of 2).
// `keys[i] == null` marks an empty slot — terminates probe sequences.
// Null keys are forbidden by the public API, so this sentinel is unambiguous.
class HashMap<K,V> implements Map<K,V> {
    K[] keys;
    V[] values;
    int[] hashes;
    int capacity;
    int count;

    public constructor() {
        this.capacity = 32;
        this.keys = new K[this.capacity];
        this.values = new V[this.capacity];
        this.hashes = new int[this.capacity];
        this.count = 0;
    }

    // initialCapacity must be a power of two >= 4 (caller responsibility).
    public constructor(int initialCapacity) {
        this.capacity = initialCapacity;
        this.keys = new K[this.capacity];
        this.values = new V[this.capacity];
        this.hashes = new int[this.capacity];
        this.count = 0;
    }

    public function put(K key, V value): V {
        if (key == null) {
            print("Error: HashMap.put() - key cannot be null");
            return null;
        }

        int rawHash = key.hashCode();
        int mixed = rawHash * 1610612741;
        int mask = this.capacity - 1;
        int idx = (mixed ^ (mixed >> 16)) & mask;

        while (this.keys[idx] != null) {
            if (this.hashes[idx] == rawHash && this.keys[idx].equals(key)) {
                V oldValue = this.values[idx];
                this.values[idx] = value;
                return oldValue;
            }
            idx = (idx + 1) & mask;
        }

        this.keys[idx] = key;
        this.values[idx] = value;
        this.hashes[idx] = rawHash;
        this.count++;

        if (this.count > this.capacity * 3 / 4) {
            this.resize();
        }
        return null;
    }

    public function get(K key): V {
        if (key == null) {
            print("Error: HashMap.get() - key cannot be null");
            return null;
        }

        int rawHash = key.hashCode();
        int mixed = rawHash * 1610612741;
        int mask = this.capacity - 1;
        int idx = (mixed ^ (mixed >> 16)) & mask;

        while (this.keys[idx] != null) {
            if (this.hashes[idx] == rawHash && this.keys[idx].equals(key)) {
                return this.values[idx];
            }
            idx = (idx + 1) & mask;
        }
        return null;
    }

    public function containsKey(K key): bool {
        if (key == null) {
            print("Error: HashMap.containsKey() - key cannot be null");
            return false;
        }

        int rawHash = key.hashCode();
        int mixed = rawHash * 1610612741;
        int mask = this.capacity - 1;
        int idx = (mixed ^ (mixed >> 16)) & mask;

        while (this.keys[idx] != null) {
            if (this.hashes[idx] == rawHash && this.keys[idx].equals(key)) {
                return true;
            }
            idx = (idx + 1) & mask;
        }
        return false;
    }

    public function remove(K key): bool {
        if (key == null) {
            print("Error: HashMap.remove() - key cannot be null");
            return false;
        }

        int rawHash = key.hashCode();
        int mixed = rawHash * 1610612741;
        int mask = this.capacity - 1;
        int idx = (mixed ^ (mixed >> 16)) & mask;

        while (this.keys[idx] != null) {
            if (this.hashes[idx] == rawHash && this.keys[idx].equals(key)) {
                // Back-shift: walk forward; for each entry j past idx, check if
                // its ideal slot is in the cyclic range (i, j]. If yes, leave
                // it (it's already in a valid position past the hole). If no,
                // shift it back to fill the hole and advance the hole to j.
                int i = idx;
                while (true) {
                    int j = (i + 1) & mask;
                    if (this.keys[j] == null) {
                        this.keys[i] = null;
                        this.count--;
                        return true;
                    }
                    int hj = this.hashes[j];
                    int mj = hj * 1610612741;
                    int kIdeal = (mj ^ (mj >> 16)) & mask;

                    bool inBetween = false;
                    if (i <= j) {
                        inBetween = (kIdeal > i && kIdeal <= j);
                    } else {
                        inBetween = (kIdeal > i || kIdeal <= j);
                    }

                    if (inBetween) {
                        i = j;
                    } else {
                        this.keys[i] = this.keys[j];
                        this.values[i] = this.values[j];
                        this.hashes[i] = this.hashes[j];
                        i = j;
                    }
                }
            }
            idx = (idx + 1) & mask;
        }
        return false;
    }

    public function size(): int {
        return this.count;
    }

    public function empty(): bool {
        return this.count == 0;
    }

    public function clear(): void {
        for (int i = 0; i < this.capacity; i++) {
            this.keys[i] = null;
        }
        this.count = 0;
    }

    public function getKeys(): K[] {
        K[] result = new K[this.count];
        int index = 0;
        for (int i = 0; i < this.capacity; i++) {
            if (this.keys[i] != null) {
                result[index] = this.keys[i];
                index++;
            }
        }
        return result;
    }

    public function getValues(): V[] {
        V[] result = new V[this.count];
        int index = 0;
        for (int i = 0; i < this.capacity; i++) {
            if (this.keys[i] != null) {
                result[index] = this.values[i];
                index++;
            }
        }
        return result;
    }

    public function toArray(): K[] {
        return this.getKeys();
    }

    public function hashCode(): int {
        int hash = 0;
        for (int i = 0; i < this.capacity; i++) {
            if (this.keys[i] != null) {
                int keyHash = hashCode(this.keys[i]);
                int valueHash = hashCode(this.values[i]);
                hash = hash + keyHash + valueHash;
            }
        }
        return hash;
    }

    public function iterator(): Iterator<K> {
        return new HashMapKeyIterator<K>(this.getKeys());
    }

    public function entryIterator(): Iterator<MapEntry<K, V>> {
        return new HashMapEntryIterator<K, V>(this.getKeys(), this.getValues());
    }

    public function valueIterator(): Iterator<V> {
        return new HashMapValueIterator<V>(this.getValues());
    }

    public function stream(): Stream<K> {
        return new StreamImpl<K>(this.iterator());
    }

    public function streamEntries(): Stream<MapEntry<K, V>> {
        return new StreamImpl<MapEntry<K, V>>(this.entryIterator());
    }

    public function streamValues(): Stream<V> {
        return new StreamImpl<V>(this.valueIterator());
    }

    public function containsValue(V value): bool {
        for (int i = 0; i < this.capacity; i++) {
            if (this.keys[i] != null) {
                V v = this.values[i];
                if (v != null && v.equals(value)) {
                    return true;
                }
                if (v == null && value == null) {
                    return true;
                }
            }
        }
        return false;
    }

    public function putAll(Map<K,V> other): void {
        K[] otherKeys = other.getKeys();
        for (K key : otherKeys) {
            this.put(key, other.get(key));
        }
    }

    function resize(): void {
        K[] oldKeys = this.keys;
        V[] oldValues = this.values;
        int[] oldHashes = this.hashes;
        int oldCapacity = this.capacity;

        this.capacity = this.capacity * 2;
        this.keys = new K[this.capacity];
        this.values = new V[this.capacity];
        this.hashes = new int[this.capacity];
        this.count = 0;

        int mask = this.capacity - 1;
        for (int slot = 0; slot < oldCapacity; slot++) {
            K key = oldKeys[slot];
            if (key != null) {
                int rawHash = oldHashes[slot];
                int mixed = rawHash * 1610612741;
                int idx = (mixed ^ (mixed >> 16)) & mask;
                while (this.keys[idx] != null) {
                    idx = (idx + 1) & mask;
                }
                this.keys[idx] = key;
                this.values[idx] = oldValues[slot];
                this.hashes[idx] = rawHash;
                this.count++;
            }
        }
    }
}
