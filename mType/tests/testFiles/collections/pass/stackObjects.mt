// Stack with objects test

class Task {
    string name;
    int priority;
    
    constructor(string n, int p) {
        name = n;
        priority = p;
    }
    
    function getName(): string {
        return name;
    }
    
    function getPriority(): int {
        return priority;
    }
}

// Create stack of tasks
Stack<Task> tasks = new Stack<Task>();

// Push tasks
Task task1 = new Task("First task", 1);
Task task2 = new Task("Second task", 2);
Task task3 = new Task("Third task", 3);

tasks.push(task1);
tasks.push(task2);
tasks.push(task3);

print(tasks.size()); // 3

// Check top task
Task topTask = tasks.top();
print(topTask.getName());     // "Third task"
print(topTask.getPriority()); // 3

// Pop tasks in LIFO order
Task popped1 = tasks.pop();
print(popped1.getName());     // "Third task"

Task popped2 = tasks.pop();
print(popped2.getName());     // "Second task"

Task popped3 = tasks.pop();
print(popped3.getName());     // "First task"

print(tasks.empty()); // true

print("Stack objects test passed");