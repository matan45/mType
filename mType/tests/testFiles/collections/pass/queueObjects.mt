// Queue with objects test

class Customer {
    string name;
    int orderNum;
    
    constructor(string n, int order) {
        name = n;
        orderNum = order;
    }
    
    function getName(): string {
        return name;
    }
    
    function getOrderNum(): int {
        return orderNum;
    }
}

// Create queue of customers
Queue<Customer> customers = new Queue<Customer>();

// Enqueue customers
Customer alice = new Customer("Alice", 101);
Customer bob = new Customer("Bob", 102);
Customer charlie = new Customer("Charlie", 103);

customers.enqueue(alice);
customers.enqueue(bob);
customers.enqueue(charlie);

print(customers.size()); // 3

// Check front customer
Customer frontCustomer = customers.front();
print(frontCustomer.getName());     // "Alice"
print(frontCustomer.getOrderNum()); // 101

// Serve customers in FIFO order
Customer served1 = customers.dequeue();
print(served1.getName());     // "Alice"

Customer served2 = customers.dequeue();
print(served2.getName());     // "Bob"

Customer served3 = customers.dequeue();
print(served3.getName());     // "Charlie"

print(customers.empty()); // true

print("Queue objects test passed");