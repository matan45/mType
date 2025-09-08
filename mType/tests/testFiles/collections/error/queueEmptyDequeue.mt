// Queue empty dequeue error test

Queue<string> queue = new Queue<string>();

// This should cause a runtime error
queue.dequeue(); // Error: Cannot dequeue from empty queue