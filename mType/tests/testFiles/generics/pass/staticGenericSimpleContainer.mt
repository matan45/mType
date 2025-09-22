import "../../complie/lib/primitives/String.mt";
import "../../complie/lib/primitives/Int.mt";

// Simple generic class for testing static generic methods
class SimpleContainer<T> {
    T value;

    constructor(T val) {
        value = val;
    }

    function getValue(): T {
        return value;
    }
}

class ContainerFactory {
    // Static generic method that returns a simple generic class
    static function <T> createContainer(T item): SimpleContainer<T> {
        return new SimpleContainer<T>(item);
    }

    // Static generic method that creates and accesses the container
    static function <T> createAndAccess(T item): T {
        SimpleContainer<T> container = new SimpleContainer<T>(item);
        return container.getValue();
    }
}

// Test static generic method returning generic class
SimpleContainer<String> stringContainer = ContainerFactory::createContainer<String>(new String("test"));
print("String container created successfully");

SimpleContainer<Int> intContainer = ContainerFactory::createContainer<Int>(new Int(42));
print("Int container created successfully");

// Test static generic method with local object manipulation
String result1 = ContainerFactory::createAndAccess<String>(new String("hello"));
print("String result: " + result1.toString());

Int result2 = ContainerFactory::createAndAccess<Int>(new Int(99));
print("Int result: " + result2.toString());

print("Static generic simple container tests passed");