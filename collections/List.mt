// List<T> - Dynamic resizable list implementation
class List<T> {
    T[] data;
    int capacity;
    int count;

    // Constructor
    constructor() {
            this.capacity = 4;
            this.data = new T[this.capacity];
            this.count = 0;
        }

        // Core operations
        function add(T item): void {
            if (this.count >= this.capacity) {
                this.resize();
            }
            this.data[this.count] = item;
            this.count++;
        }

        function get(int index): T {
            if (index < 0 || index >= this.count) {
                // TODO: Error handling - for now return default
                return null;
            }
            return this.data[index];
        }

        function set(int index, T item): void {
            if (index < 0 || index >= this.count) {
                // TODO: Error handling
                return;
            }
            this.data[index] = item;
        }

        function contains(T item): bool {
            for (int i = 0; i < this.count; i++) {
                if (this.data[i] == item) {
                    return true;
                }
            }
            return false;
        }

        // Content-based comparison using equals() method
        function containsEquals(T item): bool {
            for (int i = 0; i < this.count; i++) {
                if (this.data[i].equals(item)) {
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
            // Keep capacity for efficiency
        }

        // Remove element at index
        function removeAt(int index): bool {
            if (index < 0 || index >= this.count) {
                return false;
            }

            // Shift elements left
            for (int i = index; i < this.count - 1; i++) {
                this.data[i] = this.data[i + 1];
            }
            this.count--;
            return true;
        }

        // Remove first occurrence of item (identity comparison)
        function remove(T item): bool {
            for (int i = 0; i < this.count; i++) {
                if (this.data[i] == item) {
                    return this.removeAt(i);
                }
            }
            return false;
        }

        // Remove first occurrence of item (content comparison using equals)
        function removeEquals(T item): bool {
            for (int i = 0; i < this.count; i++) {
                if (this.data[i].equals(item)) {
                    return this.removeAt(i);
                }
            }
            return false;
        }

        // Get first element
        function first(): T {
            if (this.count == 0) {
                return null;
            }
            return this.data[0];
        }

        // Get last element
        function last(): T {
            if (this.count == 0) {
                return null;
            }
            return this.data[this.count - 1];
        }

        // Helper method
        function resize(): void {
            this.capacity = this.capacity * 2;
            T[] newData = new T[this.capacity];
            for (int i = 0; i < this.count; i++) {
                newData[i] = this.data[i];
            }
            this.data = newData;
        }
}