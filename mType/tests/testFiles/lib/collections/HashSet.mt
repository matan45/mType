// HashSet<T> - Open-addressing hash table with linear probing on flat 1D arrays
import * from "../../lib/interfaces/Set.mt";
import * from "../../lib/Iterator.mt";
import * from "../../lib/iterators/HashSetIterator.mt";
import * from "../../lib/stream/Stream.mt";
import * from "../../lib/stream/StreamImpl.mt";

// Storage layout: 2 parallel flat arrays of length `capacity` (power of 2).
// `elements[i] == null` marks an empty slot — terminates probe sequences.
// Null elements are forbidden by the public API, so this sentinel is unambiguous.
class HashSet<T> implements Set<T> {
    T[] elements;
    int[] hashes;
    int capacity;
    int count;

    public constructor() {
        this.capacity = 32;
        this.elements = new T[this.capacity];
        this.hashes = new int[this.capacity];
        this.count = 0;
    }

    // initialCapacity must be a power of two >= 4 (caller responsibility).
    public constructor(int initialCapacity) {
        this.capacity = initialCapacity;
        this.elements = new T[this.capacity];
        this.hashes = new int[this.capacity];
        this.count = 0;
    }

    public function add(T item): bool {
        if (item == null) {
            print("Error: HashSet.add() - item cannot be null");
            return false;
        }

        int rawHash = item.hashCode();
        int mixed = rawHash * 1610612741;
        int mask = this.capacity - 1;
        int idx = (mixed ^ (mixed >> 16)) & mask;

        while (this.elements[idx] != null) {
            if (this.hashes[idx] == rawHash && this.elements[idx].equals(item)) {
                return false;
            }
            idx = (idx + 1) & mask;
        }

        this.elements[idx] = item;
        this.hashes[idx] = rawHash;
        this.count++;

        if (this.count > this.capacity * 3 / 4) {
            this.resize();
        }
        return true;
    }

    public function contains(T item): bool {
        if (item == null) {
            print("Error: HashSet.contains() - item cannot be null");
            return false;
        }

        int rawHash = item.hashCode();
        int mixed = rawHash * 1610612741;
        int mask = this.capacity - 1;
        int idx = (mixed ^ (mixed >> 16)) & mask;

        while (this.elements[idx] != null) {
            if (this.hashes[idx] == rawHash && this.elements[idx].equals(item)) {
                return true;
            }
            idx = (idx + 1) & mask;
        }
        return false;
    }

    public function remove(T item): bool {
        if (item == null) {
            print("Error: HashSet.remove() - item cannot be null");
            return false;
        }

        int rawHash = item.hashCode();
        int mixed = rawHash * 1610612741;
        int mask = this.capacity - 1;
        int idx = (mixed ^ (mixed >> 16)) & mask;

        while (this.elements[idx] != null) {
            if (this.hashes[idx] == rawHash && this.elements[idx].equals(item)) {
                int i = idx;
                while (true) {
                    int j = (i + 1) & mask;
                    if (this.elements[j] == null) {
                        this.elements[i] = null;
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
                        this.elements[i] = this.elements[j];
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
            this.elements[i] = null;
        }
        this.count = 0;
    }

    public function toArray(): T[] {
        T[] result = new T[this.count];
        int index = 0;
        for (int i = 0; i < this.capacity; i++) {
            if (this.elements[i] != null) {
                result[index] = this.elements[i];
                index++;
            }
        }
        return result;
    }

    public function hashCode(): int {
        int hash = 0;
        for (int i = 0; i < this.capacity; i++) {
            if (this.elements[i] != null) {
                hash = hash + hashCode(this.elements[i]);
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

    function resize(): void {
        T[] oldElements = this.elements;
        int[] oldHashes = this.hashes;
        int oldCapacity = this.capacity;

        this.capacity = this.capacity * 2;
        this.elements = new T[this.capacity];
        this.hashes = new int[this.capacity];
        this.count = 0;

        int mask = this.capacity - 1;
        for (int slot = 0; slot < oldCapacity; slot++) {
            T item = oldElements[slot];
            if (item != null) {
                int rawHash = oldHashes[slot];
                int mixed = rawHash * 1610612741;
                int idx = (mixed ^ (mixed >> 16)) & mask;
                while (this.elements[idx] != null) {
                    idx = (idx + 1) & mask;
                }
                this.elements[idx] = item;
                this.hashes[idx] = rawHash;
                this.count++;
            }
        }
    }
}
