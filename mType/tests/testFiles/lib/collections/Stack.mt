// Stack<T> - LIFO operations
import * from "../../lib/interfaces/Deque.mt";
import * from "../../lib/Iterator.mt";
import * from "../../lib/iterators/StackIterator.mt";
import * from "../../lib/stream/Stream.mt";
import * from "../../lib/stream/StreamImpl.mt";

class Stack<T> implements Deque<T> {
    T[] data;
    int capacity;
    int top;
        constructor() {
            this.capacity = 4;
            this.data = new T[this.capacity];
            this.top = -1;
        }

        public function push(T item): void {
            if (this.top + 1 >= this.capacity) {
                this.resize();
            }
            this.top++;
            this.data[this.top] = item;
        }

        public function pop(): T {
            if (this.empty()) {
                return null; // Stack empty
            }
            T item = this.data[this.top];
            this.top--;
            return item;
        }

        public function peek(): T {
            if (this.empty()) {
                return null; // Stack empty
            }
            return this.data[this.top];
        }

        public function empty(): bool {
            return this.top < 0;
        }

        public function size(): int {
            return this.top + 1;
        }

        public function clear(): void {
            this.top = -1;
        }

        // Check if stack contains item
        public function contains(T item): bool {
            T[] currentData = this.toArray();
            for (T element : currentData) {
                if (element.equals(item)) {
                    return true;
                }
            }
            return false;
        }

        // Convert to array (bottom to top)
        public function toArray(): T[] {
            T[] result = new T[this.top + 1];
            for (int i = 0; i <= this.top; i++) {
                result[i] = this.data[i];
            }
            return result;
        }

        // Content-based hash code (order matters for Stack)
        public function hashCode(): int {
            int hash = 1;
            for (int i = 0; i <= this.top; i++) {
                hash = 31 * hash + hashCode(this.data[i]);
            }
            return hash;
        }

        // Iterator support - NEW for enhanced for-loop
        public function iterator(): Iterator<T> {
            return new StackIterator<T>(this.data, this.top);
        }

        // Stream support - NEW for functional programming
        public function stream(): Stream<T> {
            return new StreamImpl<T>(this.iterator());
        }

        // Collection interface methods - NEW
        public function add(T item): bool {
            this.push(item);
            return true;
        }

        public function remove(T item): bool {
            // Stack doesn't support arbitrary removal
            // Would need to pop items until found
            print("Warning: Stack.remove() not efficiently supported");
            return false;
        }

        public function addAll(T[] items): void {
            for (T item : items) {
                this.push(item);
            }
        }

        // Deque interface methods - NEW
        public function addFirst(T item): void {
            this.push(item);
        }

        public function removeFirst(): T {
            return this.pop();
        }

        public function peekFirst(): T {
            return this.peek();
        }

        public function addLast(T item): void {
            // Not efficient for stack, but required by interface
            print("Warning: Stack.addLast() not efficiently supported");
        }

        public function removeLast(): T {
            // Not efficient for stack
            print("Warning: Stack.removeLast() not efficiently supported");
            return null;
        }

        public function peekLast(): T {
            // Bottom of stack
            if (this.empty()) {
                return null;
            }
            return this.data[0];
        }

        public function enqueue(T item): void {
            this.push(item);
        }

        public function dequeue(): T {
            return this.pop();
        }

        function resize(): void {
            this.capacity = this.capacity * 2;
            T[] newData = new T[this.capacity];
            for (int i = 0; i <= this.top; i++) {
                newData[i] = this.data[i];
            }
            this.data = newData;
        }
}