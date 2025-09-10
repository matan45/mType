// Stack iteration test (bottom to top)

Stack<string> stack = new Stack<string>();

// Push elements
stack.push("bottom");
stack.push("middle");
stack.push("top");

print(stack.size()); // 3

// Iterate through stack (bottom to top)
for (string item : stack) {
    print(item);
}

// Stack should be unchanged after iteration
print(stack.top());  // "top"
print(stack.size()); // 3

// Clear the stack
stack.clear();
print(stack.empty()); // true
print(stack.size());  // 0

print("Stack iteration test passed");