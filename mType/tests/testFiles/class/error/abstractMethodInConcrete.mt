// ERROR: Non-abstract class with abstract method

// ERROR: Class is not abstract but has abstract method
class ConcreteClass {
    int value;

    public function ConcreteClass() {
        value = 42;
    }

    // ERROR: Abstract method in non-abstract class
    public abstract function process(): string;
}

function main(): void {
    ConcreteClass obj = new ConcreteClass();
    print("This should not execute");
}

main();
