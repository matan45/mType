// Set<T> - Unique elements using array
class Set<T> {
    T[] data;
    int capacity;
    int count;
        constructor() {
            this.capacity = 4;
            this.data = new T[this.capacity];
            this.count = 0;
        }

        function add(T item): bool {
            if (this.contains(item)) {
                return false; // Already exists
            }

            if (this.count >= this.capacity) {
                this.resize();
            }
            this.data[this.count] = item;
            this.count++;
            return true;
        }

        function contains(T item): bool {
            T[] currentData = this.toArray();
            for (T element : currentData) {
                if (element.equals(item)) {
                    return true;
                }
            }
            return false;
        }

        function remove(T item): bool {
            for (int i = 0; i < this.count; i++) {
                if (this.data[i].equals(item)) {
                    // Shift elements left
                    for (int j = i; j < this.count - 1; j++) {
                        this.data[j] = this.data[j + 1];
                    }
                    this.count--;
                    return true;
                }
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
            this.count = 0;
        }

        // Convert to array
        function toArray(): T[] {
            T[] result = new T[this.count];
            for (int i = 0; i < this.count; i++) {
                result[i] = this.data[i];
            }
            return result;
        }

        // Content-based hash code (order-independent for Set)
        function hashCode(): int {
            int hash = 0;
            for (int i = 0; i < this.count; i++) {
                hash = hash + hashCode(this.data[i]); // Addition is commutative
            }
            return hash;
        }

        // Union with another set
        // NOTE: Currently has internal type resolution issue with generic instantiation
        // The method signature works correctly, but creating new Set<T> inside the method
        // doesn't properly resolve T to the concrete type. Use static helper functions as workaround.
        function union(Set<T> other): Set<T> {
            Set<T> result = new Set<T>();

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

        function resize(): void {
            this.capacity = this.capacity * 2;
            T[] newData = new T[this.capacity];
            for (int i = 0; i < this.count; i++) {
                newData[i] = this.data[i];
            }
            this.data = newData;
        }
}