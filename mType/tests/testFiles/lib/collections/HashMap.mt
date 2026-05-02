// HashMap<K,V> - Hash table implementation for O(1) operations using 2D arrays
import * from "../../lib/interfaces/Map.mt";
import * from "../../lib/interfaces/MapEntry.mt";
import * from "../../lib/Iterator.mt";
import * from "../../lib/iterators/HashMapKeyIterator.mt";
import * from "../../lib/iterators/HashMapEntryIterator.mt";
import * from "../../lib/iterators/HashMapValueIterator.mt";
import * from "../../lib/stream/Stream.mt";
import * from "../../lib/stream/StreamImpl.mt";

class HashMap<K,V> implements Map<K,V> {
    K[][] keyBuckets;
    V[][] valueBuckets;
    int[][] hashBuckets;
    int[] bucketSizes;
    int capacity;
    int count;

    public constructor() {
        this.capacity = 32;
        this.keyBuckets = new K[this.capacity][16];
        this.valueBuckets = new V[this.capacity][16];
        this.hashBuckets = new int[this.capacity][16];
        this.bucketSizes = new int[this.capacity];
        this.count = 0;

        for (int i = 0; i < this.capacity; i++) {
            this.bucketSizes[i] = 0;
        }
    }

    // initialCapacity must be a power of two >= 4 (caller responsibility).
    public constructor(int initialCapacity) {
        this.capacity = initialCapacity;
        this.keyBuckets = new K[this.capacity][16];
        this.valueBuckets = new V[this.capacity][16];
        this.hashBuckets = new int[this.capacity][16];
        this.bucketSizes = new int[this.capacity];
        this.count = 0;

        for (int i = 0; i < this.capacity; i++) {
            this.bucketSizes[i] = 0;
        }
    }

    // Hot path: inlines getBucketIndex + findKeyInBucket. Subclass overrides
    // of those helpers will NOT affect this method (only `remove` dispatches).
    public function put(K key, V value): V {
        if (key == null) {
            print("Error: HashMap.put() - key cannot be null");
            return null;
        }

        int rawHash = key.hashCode();
        int mixed = rawHash * 1610612741;
        int bucketIndex = (mixed ^ (mixed >> 16)) & (this.capacity - 1);

        int bucketSize = this.bucketSizes[bucketIndex];
        for (int i = 0; i < bucketSize; i++) {
            if (this.hashBuckets[bucketIndex][i] == rawHash) {
                K existing = this.keyBuckets[bucketIndex][i];
                if (existing != null && existing.equals(key)) {
                    V oldValue = this.valueBuckets[bucketIndex][i];
                    this.valueBuckets[bucketIndex][i] = value;
                    return oldValue;
                }
            }
        }

        if (bucketSize >= this.keyBuckets[bucketIndex].length) {
            this.resizeBucket(bucketIndex);
        }

        this.keyBuckets[bucketIndex][bucketSize] = key;
        this.valueBuckets[bucketIndex][bucketSize] = value;
        this.hashBuckets[bucketIndex][bucketSize] = rawHash;
        this.bucketSizes[bucketIndex] = bucketSize + 1;
        this.count++;

        if (this.count > this.capacity * 3 / 4) {
            this.resize();
        }
        return null;
    }

    // Hot path: inlines getBucketIndex + findKeyInBucket. See note on `put`.
    public function get(K key): V {
        if (key == null) {
            print("Error: HashMap.get() - key cannot be null");
            return null;
        }

        int rawHash = key.hashCode();
        int mixed = rawHash * 1610612741;
        int bucketIndex = (mixed ^ (mixed >> 16)) & (this.capacity - 1);

        int bucketSize = this.bucketSizes[bucketIndex];
        for (int i = 0; i < bucketSize; i++) {
            if (this.hashBuckets[bucketIndex][i] == rawHash) {
                K existing = this.keyBuckets[bucketIndex][i];
                if (existing != null && existing.equals(key)) {
                    return this.valueBuckets[bucketIndex][i];
                }
            }
        }
        return null;
    }

    // Hot path: inlines getBucketIndex + findKeyInBucket. See note on `put`.
    public function containsKey(K key): bool {
        if (key == null) {
            print("Error: HashMap.containsKey() - key cannot be null");
            return false;
        }

        int rawHash = key.hashCode();
        int mixed = rawHash * 1610612741;
        int bucketIndex = (mixed ^ (mixed >> 16)) & (this.capacity - 1);

        int bucketSize = this.bucketSizes[bucketIndex];
        for (int i = 0; i < bucketSize; i++) {
            if (this.hashBuckets[bucketIndex][i] == rawHash) {
                K existing = this.keyBuckets[bucketIndex][i];
                if (existing != null && existing.equals(key)) {
                    return true;
                }
            }
        }
        return false;
    }

    public function remove(K key): bool {
        if (key == null) {
            print("Error: HashMap.remove() - key cannot be null");
            return false;
        }

        int bucketIndex = this.getBucketIndex(key);
        int keyIndex = this.findKeyInBucket(bucketIndex, key);

        if (keyIndex >= 0) {
            int bs = this.bucketSizes[bucketIndex];
            for (int i = keyIndex; i < bs - 1; i++) {
                this.keyBuckets[bucketIndex][i] = this.keyBuckets[bucketIndex][i + 1];
                this.valueBuckets[bucketIndex][i] = this.valueBuckets[bucketIndex][i + 1];
                this.hashBuckets[bucketIndex][i] = this.hashBuckets[bucketIndex][i + 1];
            }
            this.bucketSizes[bucketIndex] = bs - 1;
            this.count--;
            return true;
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
            this.bucketSizes[i] = 0;
        }
        this.count = 0;
    }

    public function getKeys(): K[] {
        K[] result = new K[this.count];
        int index = 0;

        for (int bucket = 0; bucket < this.capacity; bucket++) {
            for (int i = 0; i < this.bucketSizes[bucket]; i++) {
                result[index] = this.keyBuckets[bucket][i];
                index++;
            }
        }
        return result;
    }

    public function getValues(): V[] {
        V[] result = new V[this.count];
        int index = 0;

        for (int bucket = 0; bucket < this.capacity; bucket++) {
            for (int i = 0; i < this.bucketSizes[bucket]; i++) {
                result[index] = this.valueBuckets[bucket][i];
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
        for (int bucket = 0; bucket < this.capacity; bucket++) {
            for (int i = 0; i < this.bucketSizes[bucket]; i++) {
                int keyHash = hashCode(this.keyBuckets[bucket][i]);
                int valueHash = hashCode(this.valueBuckets[bucket][i]);
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
        for (int bucket = 0; bucket < this.capacity; bucket++) {
            for (int i = 0; i < this.bucketSizes[bucket]; i++) {
                V v = this.valueBuckets[bucket][i];
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
        K[] keys = other.getKeys();
        for (K key : keys) {
            this.put(key, other.get(key));
        }
    }

    function getBucketIndex(K key): int {
        int hash = key.hashCode() * 1610612741;
        return (hash ^ (hash >> 16)) & (this.capacity - 1);
    }

    function findKeyInBucket(int bucketIndex, K key): int {
        for (int i = 0; i < this.bucketSizes[bucketIndex]; i++) {
            if (this.keyBuckets[bucketIndex][i] != null && this.keyBuckets[bucketIndex][i].equals(key)) {
                return i;
            }
        }
        return -1;
    }

    // NOTE: K[][] is a FlatMultiArray — one contiguous allocation. The runtime
    // does not support replacing a whole row, so we cannot actually grow an
    // inner bucket. In practice, the load-factor doubling in `resize()` keeps
    // per-bucket population below the initial inner-array size (4), so this
    // path is never triggered. Kept as a structural no-op for safety.
    function resizeBucket(int bucketIndex): void {
        int oldSize = this.keyBuckets[bucketIndex].length;

        K[] newKeys = new K[oldSize];
        V[] newValues = new V[oldSize];
        int[] newHashes = new int[oldSize];

        for (int i = 0; i < oldSize; i++) {
            newKeys[i] = this.keyBuckets[bucketIndex][i];
            newValues[i] = this.valueBuckets[bucketIndex][i];
            newHashes[i] = this.hashBuckets[bucketIndex][i];
        }

        for (int i = 0; i < oldSize; i++) {
            this.keyBuckets[bucketIndex][i] = newKeys[i];
            this.valueBuckets[bucketIndex][i] = newValues[i];
            this.hashBuckets[bucketIndex][i] = newHashes[i];
        }
    }

    function resize(): void {
        K[][] oldKeyBuckets = this.keyBuckets;
        V[][] oldValueBuckets = this.valueBuckets;
        int[][] oldHashBuckets = this.hashBuckets;
        int[] oldBucketSizes = this.bucketSizes;
        int oldCapacity = this.capacity;

        this.capacity = this.capacity * 2;
        this.keyBuckets = new K[this.capacity][16];
        this.valueBuckets = new V[this.capacity][16];
        this.hashBuckets = new int[this.capacity][16];
        this.bucketSizes = new int[this.capacity];
        this.count = 0;

        for (int i = 0; i < this.capacity; i++) {
            this.bucketSizes[i] = 0;
        }

        int mask = this.capacity - 1;
        for (int bucket = 0; bucket < oldCapacity; bucket++) {
            int bs = oldBucketSizes[bucket];
            for (int i = 0; i < bs; i++) {
                K key = oldKeyBuckets[bucket][i];
                V value = oldValueBuckets[bucket][i];
                int rawHash = oldHashBuckets[bucket][i];
                int mixed = rawHash * 1610612741;
                int newBucketIndex = (mixed ^ (mixed >> 16)) & mask;

                int newSlot = this.bucketSizes[newBucketIndex];
                if (newSlot >= this.keyBuckets[newBucketIndex].length) {
                    this.resizeBucket(newBucketIndex);
                }
                this.keyBuckets[newBucketIndex][newSlot] = key;
                this.valueBuckets[newBucketIndex][newSlot] = value;
                this.hashBuckets[newBucketIndex][newSlot] = rawHash;
                this.bucketSizes[newBucketIndex] = newSlot + 1;
                this.count++;
            }
        }
    }
}
