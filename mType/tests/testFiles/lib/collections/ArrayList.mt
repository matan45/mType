// List<T> - Dynamic resizable list implementation
import * from "../../lib/interfaces/List.mt";
import * from "../../lib/Iterator.mt";
import * from "../../lib/iterators/ListIterator.mt";
import * from "../../lib/stream/Stream.mt";
import * from "../../lib/stream/StreamImpl.mt";

class ArrayList<T> implements List<T> {
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
        public function add(T item): bool {
            // Null item validation
            if (item == null) {
                print("Error: List.add() - item cannot be null");
                return false;
            }

            if (this.count >= this.capacity) {
                this.resize();
            }
            this.data[this.count] = item;
            this.count++;
            return true;
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

        // Iterator support - NEW for enhanced for-loop
        public function iterator(): Iterator<T> {
            return new ListIterator<T>(this.data, this.count);
        }

        // Stream support - NEW for functional programming
        public function stream(): Stream<T> {
            return new StreamImpl<T>(this.iterator());
        }

        // Search operations - NEW
        public function indexOf(T item): int {
            if (item == null) {
                return -1;
            }
            for (int i = 0; i < this.count; i++) {
                if (this.data[i] != null && this.data[i].equals(item)) {
                    return i;
                }
            }
            return -1;
        }

        public function lastIndexOf(T item): int {
            if (item == null) {
                return -1;
            }
            for (int i = this.count - 1; i >= 0; i = i - 1) {
                if (this.data[i] != null && this.data[i].equals(item)) {
                    return i;
                }
            }
            return -1;
        }

        // Bulk operations - NEW
        public function addAll(T[] items): void {
            for (T item : items) {
                this.add(item);
            }
        }

        // List manipulation - NEW
        public function reverse(): void {
            int left = 0;
            int right = this.count - 1;
            while (left < right) {
                T temp = this.data[left];
                this.data[left] = this.data[right];
                this.data[right] = temp;
                left = left + 1;
                right = right - 1;
            }
        }

        // Sort - Placeholder (requires Comparable interface)
        public function sort(): void {
            // TODO: Implement sorting algorithm
            // For now, this is a placeholder
            print("Warning: List.sort() not yet implemented");
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