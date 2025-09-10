// Basic queue creation and usage (FIFO)

// Create an empty queue
Queue<int> numbers = new Queue<int>();

print(numbers.empty()); // true
print(numbers.size());  // 0

// Enqueue elements
numbers.enqueue(10);
numbers.enqueue(20);
numbers.enqueue(30);

print(numbers.size());  // 3
print(numbers.empty()); // false

// Peek at front element
print(numbers.front()); // 10 (doesn't remove)
print(numbers.size());  // 3 (size unchanged)

// Dequeue elements (FIFO order)
print(numbers.dequeue()); // 10
print(numbers.dequeue()); // 20
print(numbers.size());    // 1

print(numbers.front());   // 30
print(numbers.dequeue()); // 30
print(numbers.empty());   // true

print("Queue basic test passed");