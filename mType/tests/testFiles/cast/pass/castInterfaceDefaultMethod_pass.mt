// Test interface with default method casting
// Note: mType interfaces don't have default methods in the traditional sense,
// but we can test abstract classes with default implementations

interface Greetable {
    public function greet(): string;
    public function farewell(): string;
}

abstract class BaseGreetable implements Greetable {
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
Greetable g1 = (Greetable)eng;
print(g1.greet());
print(g1.farewell());

// Test with overridden implementation
SpanishGreeting spa = new SpanishGreeting();
Greetable g2 = (Greetable)spa;
print(g2.greet());
print(g2.farewell());

// Cast to base abstract class
BaseGreetable base1 = (BaseGreetable)eng;
BaseGreetable base2 = (BaseGreetable)spa;
print(base1.farewell());
print(base2.farewell());

print("Interface default method casting test passed");
