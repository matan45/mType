// Test interface with default method casting
// Note: mType interfaces don't have default methods in the traditional sense,
// but we can test abstract classes with default implementations

interface Greetable {
    public function greet(): string;
    public function farewell(): string;
}

abstract class BaseGreetable implements Greetable {
    // Abstract method - must be implemented by subclasses
    public abstract function greet(): string;

    // Provide default implementation for farewell
    public function farewell(): string {
        return "Goodbye!";
    }
}

class EnglishGreeting extends BaseGreetable {
    public function greet(): string {
        return "Hello!";
    }
}

class SpanishGreeting extends BaseGreetable {
    public function greet(): string {
        return "Hola!";
    }

    // Override default farewell
    public function farewell(): string {
        return "Adios!";
    }
}

// Test with default implementation
EnglishGreeting eng = new EnglishGreeting();
BaseGreetable base1 = eng;
print(base1.greet());
print(base1.farewell());

// Test with overridden implementation
SpanishGreeting spa = new SpanishGreeting();
BaseGreetable base2 = spa;
print(base2.greet());
print(base2.farewell());

// Test polymorphism through abstract class
print(base1.farewell());
print(base2.farewell());

print("Interface default method casting test passed");
