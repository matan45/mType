// Enhanced HashMap<K,V> with interface support and stream API
// Supports functional programming with lambdas and stream operations

// Functional interfaces for stream operations
interface Predicate<T> {
    function test(T value): bool;
}

interface Consumer<T> {
    function accept(T value): void;
}

interface Function<T, R> {
    function apply(T value): R;
}

interface BiConsumer<T, U> {
    function accept(T first, U second): void;
}

interface BiFunction<T, U, R> {
    function apply(T first, U second): R;
}

interface BiPredicate<T, U> {
    function test(T first, U second): bool;
}

// Entry class for key-value pairs
class Entry<K, V> {
    K key;
    V value;

    constructor(K key, V value) {
        this.key = key;
        this.value = value;
    }

    function getKey(): K {
        return this.key;
    }

    function getValue(): V {
        return this.value;
    }

    function setValue(V value): V {
        V oldValue = this.value;
        this.value = value;
        return oldValue;
    }

    function equals(Entry<K, V> other): bool {
        if (other == null) return false;
        return this.key.equals(other.key) && this.value.equals(other.value);
    }

    function hashCode(): int {
        int keyHash = (this.key == null) ? 0 : this.key.hashCode();
        int valueHash = (this.value == null) ? 0 : this.value.hashCode();
        return keyHash + valueHash * 31;
    }

    function toString(): string {
        return this.key.toString() + "=" + this.value.toString();
    }
}

// Stream class for functional operations
class Stream<T> {
    T[] elements;
    int size;

    constructor(T[] elements) {
        this.elements = elements;
        this.size = elements.length;
    }

    // Filter elements using a predicate
    function filter(Predicate<T> predicate): Stream<T> {
        T[] filtered = new T[this.size]; // Over-allocate, will trim later
        int count = 0;

        for (int i = 0; i < this.size; i++) {
            if (predicate.test(this.elements[i])) {
                filtered[count] = this.elements[i];
                count++;
            }
        }

        // Create properly sized array
        T[] result = new T[count];
        for (int i = 0; i < count; i++) {
            result[i] = filtered[i];
        }

        return new Stream<T>(result);
    }

    // Note: Generic map function not supported in non-static context
    // Use stream.toArray() then create new stream with different type

    // Execute action for each element
    function forEach(Consumer<T> action): void {
        for (int i = 0; i < this.size; i++) {
            action.accept(this.elements[i]);
        }
    }

    // Find first element matching predicate
    function findFirst(Predicate<T> predicate): T {
        for (int i = 0; i < this.size; i++) {
            if (predicate.test(this.elements[i])) {
                return this.elements[i];
            }
        }
        return null;
    }

    // Check if any element matches predicate
    function anyMatch(Predicate<T> predicate): bool {
        for (int i = 0; i < this.size; i++) {
            if (predicate.test(this.elements[i])) {
                return true;
            }
        }
        return false;
    }

    // Check if all elements match predicate
    function allMatch(Predicate<T> predicate): bool {
        for (int i = 0; i < this.size; i++) {
            if (!predicate.test(this.elements[i])) {
                return false;
            }
        }
        return true;
    }

    // Count elements
    function count(): int {
        return this.size;
    }

    // Convert stream back to array
    function toArray(): T[] {
        T[] result = new T[this.size];
        for (int i = 0; i < this.size; i++) {
            result[i] = this.elements[i];
        }
        return result;
    }
}

// Enhanced HashMap with functional programming support
class HashMap<K, V> {
    K[][] keyBuckets;
    V[][] valueBuckets;
    int[] bucketSizes;
    int capacity;
    int count;
    float loadFactor;

    constructor() {
        this.capacity = 16; // Power of 2 for efficient modulo
        this.loadFactor = 0.75;
        this.keyBuckets = new K[this.capacity][4]; // 16 buckets, each starts with capacity 4
        this.valueBuckets = new V[this.capacity][4];
        this.bucketSizes = new int[this.capacity];
        this.count = 0;

        // Initialize bucket sizes
        for (int i = 0; i < this.capacity; i++) {
            this.bucketSizes[i] = 0;
        }
    }

    constructor(int initialCapacity) {
        this.capacity = this.nextPowerOfTwo(initialCapacity);
        this.loadFactor = 0.75;
        this.keyBuckets = new K[this.capacity][4];
        this.valueBuckets = new V[this.capacity][4];
        this.bucketSizes = new int[this.capacity];
        this.count = 0;

        for (int i = 0; i < this.capacity; i++) {
            this.bucketSizes[i] = 0;
        }
    }

    constructor(int initialCapacity, float loadFactor) {
        this.capacity = this.nextPowerOfTwo(initialCapacity);
        this.loadFactor = loadFactor;
        this.keyBuckets = new K[this.capacity][4];
        this.valueBuckets = new V[this.capacity][4];
        this.bucketSizes = new int[this.capacity];
        this.count = 0;

        for (int i = 0; i < this.capacity; i++) {
            this.bucketSizes[i] = 0;
        }
    }

    // Core HashMap operations
    function put(K key, V value): V {
        if (key == null) {
            print("Error: HashMap.put() - key cannot be null");
            return null;
        }

        int bucketIndex = this.getBucketIndex(key);
        int keyIndex = this.findKeyInBucket(bucketIndex, key);

        if (keyIndex >= 0) {
            // Update existing key and return old value
            V oldValue = this.valueBuckets[bucketIndex][keyIndex];
            this.valueBuckets[bucketIndex][keyIndex] = value;
            return oldValue;
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

            // Resize hash table if load factor exceeds threshold
            if (this.count > this.capacity * this.loadFactor) {
                this.resize();
            }

            return null; // No previous value
        }
    }

    function get(K key): V {
        if (key == null) {
            return null;
        }

        int bucketIndex = this.getBucketIndex(key);
        int keyIndex = this.findKeyInBucket(bucketIndex, key);

        if (keyIndex >= 0) {
            return this.valueBuckets[bucketIndex][keyIndex];
        }
        return null; // Key not found
    }
	
	 // Map elements to another type using a function
    function map(Function<T, R> mapper): Stream<R> {
          R[] mapped = new R[this.size];

          for (int i = 0; i < this.size; i++) {
              mapped[i] = mapper.apply(this.elements[i]);
          }

          return new Stream<R>(mapped);
      }

    function getOrDefault(K key, V defaultValue): V {
        V value = this.get(key);
        return (value != null) ? value : defaultValue;
    }

    function containsKey(K key): bool {
        if (key == null) {
            return false;
        }

        int bucketIndex = this.getBucketIndex(key);
        return this.findKeyInBucket(bucketIndex, key) >= 0;
    }

    function containsValue(V value): bool {
        for (int bucket = 0; bucket < this.capacity; bucket++) {
            for (int i = 0; i < this.bucketSizes[bucket]; i++) {
                V bucketValue = this.valueBuckets[bucket][i];
                if ((value == null && bucketValue == null) ||
                    (value != null && value.equals(bucketValue))) {
                    return true;
                }
            }
        }
        return false;
    }

    function remove(K key): V {
        if (key == null) {
            return null;
        }

        int bucketIndex = this.getBucketIndex(key);
        int keyIndex = this.findKeyInBucket(bucketIndex, key);

        if (keyIndex >= 0) {
            V removedValue = this.valueBuckets[bucketIndex][keyIndex];

            // Shift elements left in bucket
            for (int i = keyIndex; i < this.bucketSizes[bucketIndex] - 1; i++) {
                this.keyBuckets[bucketIndex][i] = this.keyBuckets[bucketIndex][i + 1];
                this.valueBuckets[bucketIndex][i] = this.valueBuckets[bucketIndex][i + 1];
            }
            this.bucketSizes[bucketIndex] = this.bucketSizes[bucketIndex] - 1;
            this.count--;
            return removedValue;
        }
        return null;
    }

    function size(): int {
        return this.count;
    }

    function isEmpty(): bool {
        return this.count == 0;
    }

    function clear(): void {
        for (int i = 0; i < this.capacity; i++) {
            this.bucketSizes[i] = 0;
        }
        this.count = 0;
    }

    // Collection view methods
    function keySet(): K[] {
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

    function values(): V[] {
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

    function entrySet(): Entry<K, V>[] {
        Entry<K, V>[] result = new Entry<K, V>[this.count];
        int index = 0;

        for (int bucket = 0; bucket < this.capacity; bucket++) {
            for (int i = 0; i < this.bucketSizes[bucket]; i++) {
                result[index] = new Entry<K, V>(this.keyBuckets[bucket][i], this.valueBuckets[bucket][i]);
                index++;
            }
        }
        return result;
    }

    // Stream API methods
    function stream(): Stream<Entry<K, V>> {
        return new Stream<Entry<K, V>>(this.entrySet());
    }

    function keyStream(): Stream<K> {
        return new Stream<K>(this.keySet());
    }

    function valueStream(): Stream<V> {
        return new Stream<V>(this.values());
    }

    // Functional operations
    function forEach(BiConsumer<K, V> action): void {
        for (int bucket = 0; bucket < this.capacity; bucket++) {
            for (int i = 0; i < this.bucketSizes[bucket]; i++) {
                action.accept(this.keyBuckets[bucket][i], this.valueBuckets[bucket][i]);
            }
        }
    }

    function replaceAll(BiFunction<K, V, V> mapper): void {
        for (int bucket = 0; bucket < this.capacity; bucket++) {
            for (int i = 0; i < this.bucketSizes[bucket]; i++) {
                this.valueBuckets[bucket][i] = mapper.apply(
                    this.keyBuckets[bucket][i],
                    this.valueBuckets[bucket][i]
                );
            }
        }
    }

    function putIfAbsent(K key, V value): V {
        V existingValue = this.get(key);
        if (existingValue == null) {
            this.put(key, value);
            return null;
        }
        return existingValue;
    }

    function computeIfAbsent(K key, Function<K, V> mappingFunction): V {
        V value = this.get(key);
        if (value == null) {
            V newValue = mappingFunction.apply(key);
            if (newValue != null) {
                this.put(key, newValue);
            }
            return newValue;
        }
        return value;
    }

    function computeIfPresent(K key, BiFunction<K, V, V> remappingFunction): V {
        V value = this.get(key);
        if (value != null) {
            V newValue = remappingFunction.apply(key, value);
            if (newValue != null) {
                this.put(key, newValue);
            } else {
                this.remove(key);
            }
            return newValue;
        }
        return null;
    }

    function compute(K key, BiFunction<K, V, V> remappingFunction): V {
        V oldValue = this.get(key);
        V newValue = remappingFunction.apply(key, oldValue);

        if (newValue == null) {
            if (oldValue != null) {
                this.remove(key);
            }
            return null;
        } else {
            this.put(key, newValue);
            return newValue;
        }
    }

    function merge(K key, V value, BiFunction<V, V, V> remappingFunction): V {
        V oldValue = this.get(key);
        V newValue = (oldValue == null) ? value : remappingFunction.apply(oldValue, value);

        if (newValue == null) {
            this.remove(key);
        } else {
            this.put(key, newValue);
        }
        return newValue;
    }

    // Filtering methods
    function filterByKey(Predicate<K> keyPredicate): HashMap<K, V> {
        HashMap<K, V> result = new HashMap<K, V>();

        for (int bucket = 0; bucket < this.capacity; bucket++) {
            for (int i = 0; i < this.bucketSizes[bucket]; i++) {
                K key = this.keyBuckets[bucket][i];
                if (keyPredicate.test(key)) {
                    result.put(key, this.valueBuckets[bucket][i]);
                }
            }
        }
        return result;
    }

    function filterByValue(Predicate<V> valuePredicate): HashMap<K, V> {
        HashMap<K, V> result = new HashMap<K, V>();

        for (int bucket = 0; bucket < this.capacity; bucket++) {
            for (int i = 0; i < this.bucketSizes[bucket]; i++) {
                V value = this.valueBuckets[bucket][i];
                if (valuePredicate.test(value)) {
                    result.put(this.keyBuckets[bucket][i], value);
                }
            }
        }
        return result;
    }

    function filterByEntry(BiPredicate<K, V> entryPredicate): HashMap<K, V> {
        HashMap<K, V> result = new HashMap<K, V>();

        for (int bucket = 0; bucket < this.capacity; bucket++) {
            for (int i = 0; i < this.bucketSizes[bucket]; i++) {
                K key = this.keyBuckets[bucket][i];
                V value = this.valueBuckets[bucket][i];
                if (entryPredicate.test(key, value)) {
                    result.put(key, value);
                }
            }
        }
        return result;
    }

    // Note: Generic transformation methods not supported in non-static context
    // Use filtering and manual transformation instead

    // Aggregation methods
    function anyKey(Predicate<K> keyPredicate): bool {
        for (int bucket = 0; bucket < this.capacity; bucket++) {
            for (int i = 0; i < this.bucketSizes[bucket]; i++) {
                if (keyPredicate.test(this.keyBuckets[bucket][i])) {
                    return true;
                }
            }
        }
        return false;
    }

    function anyValue(Predicate<V> valuePredicate): bool {
        for (int bucket = 0; bucket < this.capacity; bucket++) {
            for (int i = 0; i < this.bucketSizes[bucket]; i++) {
                if (valuePredicate.test(this.valueBuckets[bucket][i])) {
                    return true;
                }
            }
        }
        return false;
    }

    function allKeys(Predicate<K> keyPredicate): bool {
        for (int bucket = 0; bucket < this.capacity; bucket++) {
            for (int i = 0; i < this.bucketSizes[bucket]; i++) {
                if (!keyPredicate.test(this.keyBuckets[bucket][i])) {
                    return false;
                }
            }
        }
        return true;
    }

    function allValues(Predicate<V> valuePredicate): bool {
        for (int bucket = 0; bucket < this.capacity; bucket++) {
            for (int i = 0; i < this.bucketSizes[bucket]; i++) {
                if (!valuePredicate.test(this.valueBuckets[bucket][i])) {
                    return false;
                }
            }
        }
        return true;
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

    function equals(HashMap<K, V> other): bool {
        if (other == null || this.count != other.count) {
            return false;
        }

        // Check if all entries in this map exist in other map with same values
        for (int bucket = 0; bucket < this.capacity; bucket++) {
            for (int i = 0; i < this.bucketSizes[bucket]; i++) {
                K key = this.keyBuckets[bucket][i];
                V value = this.valueBuckets[bucket][i];
                V otherValue = other.get(key);

                if ((value == null && otherValue != null) ||
                    (value != null && !value.equals(otherValue))) {
                    return false;
                }
            }
        }
        return true;
    }

    function toString(): string {
        if (this.isEmpty()) {
            return "{}";
        }

        string result = "{";
        bool first = true;

        for (int bucket = 0; bucket < this.capacity; bucket++) {
            for (int i = 0; i < this.bucketSizes[bucket]; i++) {
                if (!first) {
                    result = result + ", ";
                }
                first = false;

                K key = this.keyBuckets[bucket][i];
                V value = this.valueBuckets[bucket][i];
                result = result + key.toString() + "=" + value.toString();
            }
        }

        result = result + "}";
        return result;
    }

    // Private helper methods
    function getBucketIndex(K key): int {
        int hash = key.hashCode();

        // Handle negative values by taking absolute value
        if (hash < 0) {
            hash = -hash;
            if (hash < 0) { // Handle edge case of Integer.MIN_VALUE
                hash = hash + 1;
            }
        }

        // Apply multiplicative hash with good distribution properties
        hash = hash * 2654435761; // Large prime-like constant for mixing
        hash = hash + (hash / this.capacity); // Simple avalanche effect

        // Ensure positive and within bounds
        if (hash < 0) {
            hash = -hash;
        }

        return hash % this.capacity;
    }

    function findKeyInBucket(int bucketIndex, K key): int {
        for (int i = 0; i < this.bucketSizes[bucketIndex]; i++) {
            if (this.keyBuckets[bucketIndex][i] != null && this.keyBuckets[bucketIndex][i].equals(key)) {
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

        for (int i = 0; i < oldSize; i++) {
            this.keyBuckets[bucketIndex][i] = newKeys[i];
            this.valueBuckets[bucketIndex][i] = newValues[i];
        }
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

    function nextPowerOfTwo(int n): int {
        if (n <= 1) return 1;
        int power = 1;
        while (power < n) {
            power = power * 2;
        }
        return power;
    }
}