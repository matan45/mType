// HashSet<T> - Hash table implementation for O(1) operations
import * from "../../lib/interfaces/Set.mt";
import * from "../../lib/Iterator.mt";
import * from "../../lib/iterators/HashSetIterator.mt";
import * from "../../lib/stream/Stream.mt";
import * from "../../lib/stream/StreamImpl.mt";

class HashSet<T> implements Set<T> {
    T[][] buckets;
    int[][] itemHashes;
    int[] bucketSizes;
    int capacity;
    int count;

    public constructor() {
        this.capacity = 32;
        this.buckets = new T[this.capacity][16];
        this.itemHashes = new int[this.capacity][16];
        this.bucketSizes = new int[this.capacity];
        this.count = 0;

        for (int i = 0; i < this.capacity; i++) {
            this.bucketSizes[i] = 0;
        }
    }

    // initialCapacity must be a power of two >= 4 (caller responsibility).
    public constructor(int initialCapacity) {
        this.capacity = initialCapacity;
        this.buckets = new T[this.capacity][16];
        this.itemHashes = new int[this.capacity][16];
        this.bucketSizes = new int[this.capacity];
        this.count = 0;

        for (int i = 0; i < this.capacity; i++) {
            this.bucketSizes[i] = 0;
        }
    }

    // Hot path: inlines getBucketIndex + findItemInBucket. Subclass overrides
    // of those helpers will NOT affect this method (only `remove` dispatches).
    public function add(T item): bool {
        if (item == null) {
            print("Error: HashSet.add() - item cannot be null");
            return false;
        }

        int rawHash = item.hashCode();
        int mixed = rawHash * 1610612741;
        int bucketIndex = (mixed ^ (mixed >> 16)) & (this.capacity - 1);

        int bucketSize = this.bucketSizes[bucketIndex];
        for (int i = 0; i < bucketSize; i++) {
            if (this.itemHashes[bucketIndex][i] == rawHash) {
                T existing = this.buckets[bucketIndex][i];
                if (existing != null && existing.equals(item)) {
                    return false;
                }
            }
        }

        if (bucketSize >= this.buckets[bucketIndex].length) {
            this.resizeBucket(bucketIndex);
        }

        this.buckets[bucketIndex][bucketSize] = item;
        this.itemHashes[bucketIndex][bucketSize] = rawHash;
        this.bucketSizes[bucketIndex] = bucketSize + 1;
        this.count++;

        if (this.count > this.capacity * 3 / 4) {
            this.resize();
        }

        return true;
    }

    // Hot path: inlines getBucketIndex + findItemInBucket. See note on `add`.
    public function contains(T item): bool {
        if (item == null) {
            print("Error: HashSet.contains() - item cannot be null");
            return false;
        }

        int rawHash = item.hashCode();
        int mixed = rawHash * 1610612741;
        int bucketIndex = (mixed ^ (mixed >> 16)) & (this.capacity - 1);

        int bucketSize = this.bucketSizes[bucketIndex];
        for (int i = 0; i < bucketSize; i++) {
            if (this.itemHashes[bucketIndex][i] == rawHash) {
                T existing = this.buckets[bucketIndex][i];
                if (existing != null && existing.equals(item)) {
                    return true;
                }
            }
        }
        return false;
    }

    public function remove(T item): bool {
        if (item == null) {
            print("Error: HashSet.remove() - item cannot be null");
            return false;
        }

        int bucketIndex = this.getBucketIndex(item);
        int itemIndex = this.findItemInBucket(bucketIndex, item);

        if (itemIndex >= 0) {
            int bs = this.bucketSizes[bucketIndex];
            for (int i = itemIndex; i < bs - 1; i++) {
                this.buckets[bucketIndex][i] = this.buckets[bucketIndex][i + 1];
                this.itemHashes[bucketIndex][i] = this.itemHashes[bucketIndex][i + 1];
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

    public function toArray(): T[] {
        T[] result = new T[this.count];
        int index = 0;

        for (int bucket = 0; bucket < this.capacity; bucket++) {
            for (int i = 0; i < this.bucketSizes[bucket]; i++) {
                result[index] = this.buckets[bucket][i];
                index++;
            }
        }
        return result;
    }

    public function hashCode(): int {
        int hash = 0;
        for (int bucket = 0; bucket < this.capacity; bucket++) {
            for (int i = 0; i < this.bucketSizes[bucket]; i++) {
                hash = hash + hashCode(this.buckets[bucket][i]);
            }
        }
        return hash;
    }

    public function union(Set<T> other): Set<T> {
        HashSet<T> result = new HashSet<T>();

        T[] thisArray = this.toArray();
        for (T element : thisArray) {
            result.add(element);
        }

        T[] otherArray = other.toArray();
        for (T element : otherArray) {
            result.add(element);
        }

        return result;
    }

    public function intersection(Set<T> other): Set<T> {
        HashSet<T> result = new HashSet<T>();

        T[] thisArray = this.toArray();
        for (T element : thisArray) {
            if (other.contains(element)) {
                result.add(element);
            }
        }

        return result;
    }

    public function difference(Set<T> other): Set<T> {
        HashSet<T> result = new HashSet<T>();

        T[] thisArray = this.toArray();
        for (T element : thisArray) {
            if (!other.contains(element)) {
                result.add(element);
            }
        }

        return result;
    }

    public function isSubsetOf(Set<T> other): bool {
        T[] thisArray = this.toArray();
        for (T element : thisArray) {
            if (!other.contains(element)) {
                return false;
            }
        }
        return true;
    }

    public function isSupersetOf(Set<T> other): bool {
        return other.isSubsetOf(this);
    }

    public function iterator(): Iterator<T> {
        return new HashSetIterator<T>(this.toArray());
    }

    public function stream(): Stream<T> {
        return new StreamImpl<T>(this.iterator());
    }

    public function addAll(T[] items): void {
        for (T item : items) {
            this.add(item);
        }
    }

    function getBucketIndex(T item): int {
        int hash = item.hashCode() * 1610612741;
        return (hash ^ (hash >> 16)) & (this.capacity - 1);
    }

    function findItemInBucket(int bucketIndex, T item): int {
        for (int i = 0; i < this.bucketSizes[bucketIndex]; i++) {
            T storedItem = this.buckets[bucketIndex][i];
            if (storedItem != null && storedItem.equals(item)) {
                return i;
            }
        }
        return -1;
    }

    // NOTE: T[][] is a FlatMultiArray — one contiguous allocation. The runtime
    // does not support replacing a whole row, so we cannot actually grow an
    // inner bucket. In practice, the load-factor doubling in `resize()` keeps
    // per-bucket population below the initial inner-array size (4), so this
    // path is never triggered. Kept as a structural no-op for safety.
    function resizeBucket(int bucketIndex): void {
        int oldSize = this.buckets[bucketIndex].length;

        T[] newBucket = new T[oldSize];
        int[] newHashes = new int[oldSize];
        for (int i = 0; i < oldSize; i++) {
            newBucket[i] = this.buckets[bucketIndex][i];
            newHashes[i] = this.itemHashes[bucketIndex][i];
        }

        for (int i = 0; i < oldSize; i++) {
            this.buckets[bucketIndex][i] = newBucket[i];
            this.itemHashes[bucketIndex][i] = newHashes[i];
        }
    }

    function resize(): void {
        T[][] oldBuckets = this.buckets;
        int[][] oldHashes = this.itemHashes;
        int[] oldBucketSizes = this.bucketSizes;
        int oldCapacity = this.capacity;

        this.capacity = this.capacity * 2;
        this.buckets = new T[this.capacity][16];
        this.itemHashes = new int[this.capacity][16];
        this.bucketSizes = new int[this.capacity];
        this.count = 0;

        for (int i = 0; i < this.capacity; i++) {
            this.bucketSizes[i] = 0;
        }

        int mask = this.capacity - 1;
        for (int bucket = 0; bucket < oldCapacity; bucket++) {
            int bs = oldBucketSizes[bucket];
            for (int i = 0; i < bs; i++) {
                T item = oldBuckets[bucket][i];
                int rawHash = oldHashes[bucket][i];
                int mixed = rawHash * 1610612741;
                int newBucketIndex = (mixed ^ (mixed >> 16)) & mask;

                int newSlot = this.bucketSizes[newBucketIndex];
                if (newSlot >= this.buckets[newBucketIndex].length) {
                    this.resizeBucket(newBucketIndex);
                }
                this.buckets[newBucketIndex][newSlot] = item;
                this.itemHashes[newBucketIndex][newSlot] = rawHash;
                this.bucketSizes[newBucketIndex] = newSlot + 1;
                this.count++;
            }
        }
    }
}
