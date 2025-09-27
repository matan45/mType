// Test assignment type mismatch when calling method that returns wrong type
interface Consumer {
    function accept(int value) : void;
}

class NotAConsumer {
    bool active;
    constructor(bool a) { active = a; }
    function isActive() : bool { return active; }
}

class BadFactory {
    function createConsumer() : Consumer {
        return new NotAConsumer(true);  // Wrong return type
    }
}

print("Testing assignment from method call");
BadFactory factory = new BadFactory();
Consumer consumer = factory.createConsumer();  // Error: Object type mismatch during assignment
consumer.accept(42);  // This line won't execute
print("Consumer used successfully");