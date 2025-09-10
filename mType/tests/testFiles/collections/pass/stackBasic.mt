// Basic stack creation and usage (LIFO)

// Create an empty stack
Stack<int> numbers = new Stack<int>();

print(numbers.empty()); // true
print(numbers.size());  // 0

// Push elements onto stack
numbers.push(10);
numbers.push(20);
numbers.push(30);

print(numbers.size());  // 3
print(numbers.empty()); // false

// Peek at top element
print(numbers.top());   // 30 (doesn't remove)
print(numbers.size());  // 3 (size unchanged)

// Pop elements (LIFO order)
print(numbers.pop());   // 30
print(numbers.pop());   // 20
print(numbers.size());  // 1

print(numbers.top());   // 10
print(numbers.pop());   // 10
print(numbers.empty()); // true

print("Stack basic test passed");