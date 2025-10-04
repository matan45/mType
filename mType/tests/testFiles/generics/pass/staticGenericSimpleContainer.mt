import "../../lib/primitives/String.mt";
import "../../lib/primitives/Int.mt";

// Simple generic class for testing static generic methods
class SimpleContainer<T> {
    T value;

    public constructor(T val) {
        value = val;
    }

    public function getValue(): T {
        return value;
    }
}

class ContainerFactory {
    // Static generic method that returns a simple generic class
    public static function <T> createContainer(T item): SimpleContainer<T> {
        return new SimpleContainer<T>(item);
    }

    // Static generic method that creates and accesses the container
    public static function <T> createAndAccess(T item): T {
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