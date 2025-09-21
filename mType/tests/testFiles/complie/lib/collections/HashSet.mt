// HashSet<T> - Hash table implementation for O(1) operations
class HashSet<T> {
    T[][] buckets;
    int[] bucketSizes;
    int capacity;
    int count;

    constructor() {
        this.capacity = 16; // Power of 2 for efficient modulo
        this.buckets = new T[this.capacity][4]; // 16 buckets, each starts with capacity 4
        this.bucketSizes = new int[this.capacity];
        this.count = 0;

        // Initialize bucket sizes
        for (int i = 0; i < this.capacity; i++) {
            this.bucketSizes[i] = 0;
        }
    }

    function add(T item): bool {
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

    function contains(T item): bool {
        int bucketIndex = this.getBucketIndex(item);
        return this.findItemInBucket(bucketIndex, item) >= 0;
    }

    function remove(T item): bool {
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

    function size(): int {
        return this.count;
    }

    function empty(): bool {
        return this.count == 0;
    }

    function clear(): void {
        for (int i = 0; i < this.capacity; i++) {
            this.bucketSizes[i] = 0;
        }
        this.count = 0;
    }

    // Convert to array
    function toArray(): T[] {
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
    function hashCode(): int {
        int hash = 0;
        for (int bucket = 0; bucket < this.capacity; bucket++) {
            for (int i = 0; i < this.bucketSizes[bucket]; i++) {
                hash = hash + hashCode(this.buckets[bucket][i]); // Addition is commutative
            }
        }
        return hash;
    }

    // Union with another HashSet
    function union(HashSet<T> other): HashSet<T> {
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
    function intersection(HashSet<T> other): HashSet<T> {
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
    function difference(HashSet<T> other): HashSet<T> {
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
    function isSubsetOf(HashSet<T> other): bool {
        T[] thisArray = this.toArray();
        for (T element : thisArray) {
            if (!other.contains(element)) {
                return false;
            }
        }
        return true;
    }

    // Private helper methods
    function getBucketIndex(T item): int {
        int hash = hashCode(item);
        if (hash < 0) {
            hash = -hash; // Handle negative hash codes
        }
        return hash % this.capacity;
    }

    function findItemInBucket(int bucketIndex, T item): int {
        for (int i = 0; i < this.bucketSizes[bucketIndex]; i++) {
            if (this.buckets[bucketIndex][i].equals(item)) {
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

        this.buckets[bucketIndex] = newBucket;
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

        // Rehash all existing elements
        for (int bucket = 0; bucket < oldCapacity; bucket++) {
            for (int i = 0; i < oldBucketSizes[bucket]; i++) {
                this.add(oldBuckets[bucket][i]);
            }
        }
    }
}