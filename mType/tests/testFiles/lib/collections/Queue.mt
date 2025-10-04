// Queue<T> - FIFO operations
class Queue<T> {
    T[] data;
    int capacity;
    int front;
    int rear;
    int count;
        constructor() {
            this.capacity = 4;
            this.data = new T[this.capacity];
            this.front = 0;
            this.rear = 0;
            this.count = 0;
        }

        public function enqueue(T item): void {
            if (this.count >= this.capacity) {
                this.resize();
            }
            this.data[this.rear] = item;
            this.rear = (this.rear + 1) % this.capacity;
            this.count++;
        }

        function dequeue(): T {
            if (this.empty()) {
                return null; // Queue empty
            }
            T item = this.data[this.front];
            this.front = (this.front + 1) % this.capacity;
            this.count--;
            return item;
        }

        public function peek(): T {
            if (this.empty()) {
                return null; // Queue empty
            }
            return this.data[this.front];
        }

        function empty(): bool {
            return this.count == 0;
        }

        function size(): int {
            return this.count;
        }

        function clear(): void {
            this.front = 0;
            this.rear = 0;
            this.count = 0;
        }

        // Check if queue contains item
        function contains(T item): bool {
            T[] currentData = this.toArray();
            for (T element : currentData) {
                if (element.equals(item)) {
                    return true;
                }
            }
            return false;
        }

        // Convert to array (front to rear order)
        function toArray(): T[] {
            T[] result = new T[this.count];
            for (int i = 0; i < this.count; i++) {
                int index = (this.front + i) % this.capacity;
                result[i] = this.data[index];
            }
            return result;
        }

        // Content-based hash code (order matters for Queue)
        function hashCode(): int {
            int hash = 1;
            for (int i = 0; i < this.count; i++) {
                int index = (this.front + i) % this.capacity;
                hash = 31 * hash + hashCode(this.data[index]);
            }
            return hash;
        }

        function resize(): void {
            int oldCapacity = this.capacity;
            this.capacity = this.capacity * 2;
            T[] newData = new T[this.capacity];

            // Copy elements maintaining order
            for (int i = 0; i < this.count; i++) {
                newData[i] = this.data[(this.front + i) % oldCapacity];
            }

            this.data = newData;
            this.front = 0;
            this.rear = this.count;
        }
}