// HashSet<T> - Hash table implementation for O(1) operations
import * from "../../lib/interfaces/Set.mt";
import * from "../../lib/Iterator.mt";
import * from "../../lib/iterators/HashSetIterator.mt";
import * from "../../lib/stream/Stream.mt";
import * from "../../lib/stream/StreamImpl.mt";

class HashSet<T> implements Set<T> {
    T[][] buckets;
    int[] bucketSizes;
    int capacity;
    int count;

    public constructor() {
        this.capacity = 16; // Power of 2 for efficient modulo
        this.buckets = new T[this.capacity][4]; // 16 buckets, each starts with capacity 4
        this.bucketSizes = new int[this.capacity];
        this.count = 0;

        // Initialize bucket sizes
        for (int i = 0; i < this.capacity; i++) {
            this.bucketSizes[i] = 0;
        }
    }

    public function add(T item): bool {
        // Null item validation
        if (item == null) {
            print("Error: HashSet.add() - item cannot be null");
            return false;
        }

        int bucketIndex = this.getBucketIndex(item);

        if (this.findItemInBucket(bucketIndex, item) >= 0) {
            return false; // Already exists
        }

        // Resize bucket if needed
        if (this.bucketSizes[bucketIndex] >= this.buckets[bucketIndex].length) {
            this.resizeBucket(bucketIndex);
        }

        // Add item to bucket
        int newIndex = this.bucketSizes[bucketIndex];
        this.buckets[bucketIndex][newIndex] = item;
        this.bucketSizes[bucketIndex] = this.bucketSizes[bucketIndex] + 1;
        this.count++;

        // Resize hash table if load factor exceeds 0.75
        if (this.count > this.capacity * 3 / 4) {
            this.resize();
        }

        return true;
    }

    public function contains(T item): bool {
        // Null item validation
        if (item == null) {
            print("Error: HashSet.contains() - item cannot be null");
            return false;
        }

        int bucketIndex = this.getBucketIndex(item);
        return this.findItemInBucket(bucketIndex, item) >= 0;
    }

    public function remove(T item): bool {
        // Null item validation
        if (item == null) {
            print("Error: HashSet.remove() - item cannot be null");
            return false;
        }

        int bucketIndex = this.getBucketIndex(item);
        int itemIndex = this.findItemInBucket(bucketIndex, item);

        if (itemIndex >= 0) {
            // Shift elements left in bucket
            for (int i = itemIndex; i < this.bucketSizes[bucketIndex] - 1; i++) {
                this.buckets[bucketIndex][i] = this.buckets[bucketIndex][i + 1];
            }
            this.bucketSizes[bucketIndex] = this.bucketSizes[bucketIndex] - 1;
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

    // Convert to array
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

    // Content-based hash code (order-independent)
    public function hashCode(): int {
        int hash = 0;
        for (int bucket = 0; bucket < this.capacity; bucket++) {
            for (int i = 0; i < this.bucketSizes[bucket]; i++) {
                hash = hash + hashCode(this.buckets[bucket][i]); // Addition is commutative
            }
        }
        return hash;
    }

    // Union with another HashSet
    public function union(HashSet<T> other): HashSet<T> {
        HashSet<T> result = new HashSet<T>();

        // Add all elements from this set
        T[] thisArray = this.toArray();
        for (T element : thisArray) {
            result.add(element);
        }

        // Add all elements from other set
        T[] otherArray = other.toArray();
        for (T element : otherArray) {
            result.add(element);
        }

        return result;
    }

    // Intersection with another HashSet
    public function intersection(HashSet<T> other): HashSet<T> {
        HashSet<T> result = new HashSet<T>();

        T[] thisArray = this.toArray();
        for (T element : thisArray) {
            if (other.contains(element)) {
                result.add(element);
            }
        }

        return result;
    }

    // Difference with another HashSet (elements in this but not in other)
    public function difference(HashSet<T> other): HashSet<T> {
        HashSet<T> result = new HashSet<T>();

        T[] thisArray = this.toArray();
        for (T element : thisArray) {
            if (!other.contains(element)) {
                result.add(element);
            }
        }

        return result;
    }

    // Check if this set is a subset of another
    public function isSubsetOf(HashSet<T> other): bool {
        T[] thisArray = this.toArray();
        for (T element : thisArray) {
            if (!other.contains(element)) {
                return false;
            }
        }
        return true;
    }

    // Check if this set is a superset of another - NEW
    public function isSupersetOf(HashSet<T> other): bool {
        return other.isSubsetOf(this);
    }

    // Iterator support - NEW for enhanced for-loop
    public function iterator(): Iterator<T> {
        return new HashSetIterator<T>(this.toArray());
    }

    // Stream support - NEW for functional programming
    public function stream(): Stream<T> {
        return new StreamImpl<T>(this.iterator());
    }

    // Bulk operations - NEW
    public function addAll(T[] items): void {
        for (T item : items) {
            this.add(item);
        }
    }

    // Private helper methods
    function getBucketIndex(T item): int {
        int hash = item.hashCode();

        // Apply secondary hash mixing using only basic arithmetic
        // This is a simplified version of multiplicative hashing

        // Handle negative values by taking absolute value
        if (hash < 0) {
            hash = -hash;
            if (hash < 0) { // Handle edge case of Integer.MIN_VALUE
                hash = hash + 1;
            }
        }

        // Apply multiplicative hash with good distribution properties
        // Using a large prime for mixing
        hash = hash * 1610612741; // Large prime constant for mixing (within int range)

        // Additional mixing to reduce clustering
        hash = hash + (hash / this.capacity); // Simple avalanche effect

        // Ensure positive and within bounds
        if (hash < 0) {
            hash = -hash;
        }

        return hash % this.capacity;
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

    function resizeBucket(int bucketIndex): void {
        int oldSize = this.buckets[bucketIndex].length;
        int newSize = oldSize * 2;

        T[] newBucket = new T[newSize];
        for (int i = 0; i < oldSize; i++) {
            newBucket[i] = this.buckets[bucketIndex][i];
        }

        for (int i = 0; i < oldSize; i++) {
            this.buckets[bucketIndex][i] = newBucket[i];
        }
    }

    function resize(): void {
        // Save current state
        T[][] oldBuckets = this.buckets;
        int[] oldBucketSizes = this.bucketSizes;
        int oldCapacity = this.capacity;

        // Double capacity
        this.capacity = this.capacity * 2;
        this.buckets = new T[this.capacity][4];
        this.bucketSizes = new int[this.capacity];
        this.count = 0;

        // Initialize new bucket sizes
        for (int i = 0; i < this.capacity; i++) {
            this.bucketSizes[i] = 0;
        }

        // Rehash all existing elements without triggering further resizes
        for (int bucket = 0; bucket < oldCapacity; bucket++) {
            for (int i = 0; i < oldBucketSizes[bucket]; i++) {
                T item = oldBuckets[bucket][i];
                int newBucketIndex = this.getBucketIndex(item);

                // Resize new bucket if needed
                if (this.bucketSizes[newBucketIndex] >= this.buckets[newBucketIndex].length) {
                    this.resizeBucket(newBucketIndex);
                }

                // Add item directly without load factor check
                int newIndex = this.bucketSizes[newBucketIndex];
                this.buckets[newBucketIndex][newIndex] = item;
                this.bucketSizes[newBucketIndex] = this.bucketSizes[newBucketIndex] + 1;
                this.count++;
            }
        }
    }
}