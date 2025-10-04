// List<T> - Dynamic resizable list implementation
class List<T> {
    T[] data;
    int capacity;
    int count;

    // Constructor
    public constructor() {
            this.capacity = 10;
            this.data = new T[this.capacity];
            this.count = 0;
        }

        // Core operations
        public function add(T item): void {
            // Null item validation
            if (item == null) {
                print("Error: List.add() - item cannot be null");
                return;
            }

            if (this.count >= this.capacity) {
                this.resize();
            }
            this.data[this.count] = item;
            this.count++;
        }

        public function get(int index): T {
            if (index < 0 || index >= this.count) {
                // TODO: Error handling - for now return default
                return null;
            }
            return this.data[index];
        }

        public function set(int index, T item): void {
            if (index < 0 || index >= this.count) {
                // TODO: Error handling
                return;
            }
            // Null item validation
            if (item == null) {
                print("Error: List.set() - item cannot be null");
                return;
            }
            this.data[index] = item;
        }

        public function contains(T item): bool {
            // Null item validation
            if (item == null) {
                print("Error: List.contains() - item cannot be null");
                return false;
            }

            T[] currentData = this.toArray();
            for (T element : currentData) {
                if (element != null && element.equals(item)) {
                    return true;
                }
            }
            return false;
        }

        // Content-based comparison using equals() method
        public function containsEquals(T item): bool {
            // Null item validation
            if (item == null) {
                print("Error: List.containsEquals() - item cannot be null");
                return false;
            }

            T[] currentData = this.toArray();
            for (T element : currentData) {
                if (element != null && element.equals(item)) {
                    return true;
                }
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
            this.count = 0;
            // Keep capacity for efficiency
        }

        // Remove element at index
        public function removeAt(int index): bool {
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

        // Remove first occurrence of item (content comparison)
        public function remove(T item): bool {
            // Null item validation
            if (item == null) {
                print("Error: List.remove() - item cannot be null");
                return false;
            }

            for (int i = 0; i < this.count; i++) {
                if (this.data[i] != null && this.data[i].equals(item)) {
                    return this.removeAt(i);
                }
            }
            return false;
        }

        // Remove first occurrence of item (content comparison using equals)
        public function removeEquals(T item): bool {
            // Null item validation
            if (item == null) {
                print("Error: List.removeEquals() - item cannot be null");
                return false;
            }

            for (int i = 0; i < this.count; i++) {
                if (this.data[i] != null && this.data[i].equals(item)) {
                    return this.removeAt(i);
                }
            }
            return false;
        }

        // Get first element
        public function first(): T {
            if (this.count == 0) {
                return null;
            }
            return this.data[0];
        }

        // Get last element
        public function last(): T {
            if (this.count == 0) {
                return null;
            }
            return this.data[this.count - 1];
        }

        // Convert to array for iteration
        public function toArray(): T[] {
            T[] result = new T[this.count];
            for (int i = 0; i < this.count; i++) {
                result[i] = this.data[i];
            }
            return result;
        }

        // Content-based hash code
        public function hashCode(): int {
            int hash = 1;
            for (int i = 0; i < this.count; i++) {
                hash = 31 * hash + hashCode(this.data[i]);
            }
            return hash;
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