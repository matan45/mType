// Queue iteration test (front to back)

Queue<string> queue = new Queue<string>();

// Enqueue elements
queue.enqueue("first");
queue.enqueue("second");
queue.enqueue("third");

print(queue.size()); // 3

// Iterate through queue (front to back)
for (string item : queue) {
    print(item);
}

// Queue should be unchanged after iteration
print(queue.front()); // "first"
print(queue.size());  // 3

// Clear the queue
queue.clear();
print(queue.empty()); // true
print(queue.size());  // 0

print("Queue iteration test passed");