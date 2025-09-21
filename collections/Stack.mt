// Stack<T> - LIFO operations
class Stack<T> {
    T[] data;
    int capacity;
    int top;
        constructor() {
            this.capacity = 4;
            this.data = new T[this.capacity];
            this.top = -1;
        }

        function push(T item): void {
            if (this.top + 1 >= this.capacity) {
                this.resize();
            }
            this.top++;
            this.data[this.top] = item;
        }

        function pop(): T {
            if (this.empty()) {
                return null; // Stack empty
            }
            T item = this.data[this.top];
            this.top--;
            return item;
        }

        function peek(): T {
            if (this.empty()) {
                return null; // Stack empty
            }
            return this.data[this.top];
        }

        function empty(): bool {
            return this.top < 0;
        }

        function size(): int {
            return this.top + 1;
        }

        function clear(): void {
            this.top = -1;
        }

        // Check if stack contains item
        function contains(T item): bool {
            for (int i = 0; i <= this.top; i++) {
                if (this.data[i] == item) {
                    return true;
                }
            }
            return false;
        }

        // Convert to array (bottom to top)
        function toArray(): T[] {
            T[] result = new T[this.top + 1];
            for (int i = 0; i <= this.top; i++) {
                result[i] = this.data[i];
            }
            return result;
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