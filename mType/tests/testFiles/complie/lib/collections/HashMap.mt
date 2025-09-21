// HashMap<K,V> - Hash table implementation for O(1) operations using 2D arrays
class HashMap<K,V> {
    K[][] keyBuckets;
    V[][] valueBuckets;
    int[] bucketSizes;
    int capacity;
    int count;

    constructor() {
        this.capacity = 16; // Power of 2 for efficient modulo
        this.keyBuckets = new K[this.capacity][4]; // 16 buckets, each starts with capacity 4
        this.valueBuckets = new V[this.capacity][4];
        this.bucketSizes = new int[this.capacity];
        this.count = 0;

        // Initialize bucket sizes
        for (int i = 0; i < this.capacity; i++) {
            this.bucketSizes[i] = 0;
        }
    }

    function put(K key, V value): void {
        int bucketIndex = this.getBucketIndex(key);
        int keyIndex = this.findKeyInBucket(bucketIndex, key);

        if (keyIndex >= 0) {
            // Update existing key
            this.valueBuckets[bucketIndex][keyIndex] = value;
        } else {
            // Add new key-value pair
            if (this.bucketSizes[bucketIndex] >= this.keyBuckets[bucketIndex].length) {
                this.resizeBucket(bucketIndex);
            }

            int newIndex = this.bucketSizes[bucketIndex];
            this.keyBuckets[bucketIndex][newIndex] = key;
            this.valueBuckets[bucketIndex][newIndex] = value;
            this.bucketSizes[bucketIndex] = this.bucketSizes[bucketIndex] + 1;
            this.count++;

            // Resize hash table if load factor exceeds 0.75
            if (this.count > this.capacity * 3 / 4) {
                this.resize();
            }
        }
    }

    function get(K key): V {
        int bucketIndex = this.getBucketIndex(key);
        int keyIndex = this.findKeyInBucket(bucketIndex, key);

        if (keyIndex >= 0) {
            return this.valueBuckets[bucketIndex][keyIndex];
        }
        return null; // Key not found
    }

    function containsKey(K key): bool {
        int bucketIndex = this.getBucketIndex(key);
        return this.findKeyInBucket(bucketIndex, key) >= 0;
    }

    function remove(K key): bool {
        int bucketIndex = this.getBucketIndex(key);
        int keyIndex = this.findKeyInBucket(bucketIndex, key);

        if (keyIndex >= 0) {
            // Shift elements left in bucket
            for (int i = keyIndex; i < this.bucketSizes[bucketIndex] - 1; i++) {
                this.keyBuckets[bucketIndex][i] = this.keyBuckets[bucketIndex][i + 1];
                this.valueBuckets[bucketIndex][i] = this.valueBuckets[bucketIndex][i + 1];
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

    // Get all keys
    function getKeys(): K[] {
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

    // Get all values
    function getValues(): V[] {
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

    // Content-based hash code (order-independent)
    function hashCode(): int {
        int hash = 0;
        for (int bucket = 0; bucket < this.capacity; bucket++) {
            for (int i = 0; i < this.bucketSizes[bucket]; i++) {
                int keyHash = hashCode(this.keyBuckets[bucket][i]);
                int valueHash = hashCode(this.valueBuckets[bucket][i]);
                hash = hash + keyHash + valueHash; // Addition for order independence
            }
        }
        return hash;
    }

    // Private helper methods
    function getBucketIndex(K key): int {
        int hash = hashCode(key);
        if (hash < 0) {
            hash = -hash; // Handle negative hash codes
        }
        return hash % this.capacity;
    }

    function findKeyInBucket(int bucketIndex, K key): int {
        for (int i = 0; i < this.bucketSizes[bucketIndex]; i++) {
            if (this.keyBuckets[bucketIndex][i].equals(key)) {
                return i;
            }
        }
        return -1;
    }

    function resizeBucket(int bucketIndex): void {
        int oldSize = this.keyBuckets[bucketIndex].length;
        int newSize = oldSize * 2;

        K[] newKeys = new K[newSize];
        V[] newValues = new V[newSize];

        for (int i = 0; i < oldSize; i++) {
            newKeys[i] = this.keyBuckets[bucketIndex][i];
            newValues[i] = this.valueBuckets[bucketIndex][i];
        }

        this.keyBuckets[bucketIndex] = newKeys;
        this.valueBuckets[bucketIndex] = newValues;
    }

    function resize(): void {
        // Save current state
        K[][] oldKeyBuckets = this.keyBuckets;
        V[][] oldValueBuckets = this.valueBuckets;
        int[] oldBucketSizes = this.bucketSizes;
        int oldCapacity = this.capacity;

        // Double capacity
        this.capacity = this.capacity * 2;
        this.keyBuckets = new K[this.capacity][4];
        this.valueBuckets = new V[this.capacity][4];
        this.bucketSizes = new int[this.capacity];
        this.count = 0;

        // Initialize new bucket sizes
        for (int i = 0; i < this.capacity; i++) {
            this.bucketSizes[i] = 0;
        }

        // Rehash all existing elements
        for (int bucket = 0; bucket < oldCapacity; bucket++) {
            for (int i = 0; i < oldBucketSizes[bucket]; i++) {
                this.put(oldKeyBuckets[bucket][i], oldValueBuckets[bucket][i]);
            }
        }
    }
}