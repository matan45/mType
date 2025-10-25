// Test abstract method with generic parameters

abstract class Processor<T> {
    public abstract function <U> transform(T input, U param): U;
}

// ERROR: Must implement abstract generic method
class ConcreteProcessor extends Processor<int> {
    // Missing implementation of transform
}

function main(): void {
    ConcreteProcessor processor = new ConcreteProcessor();
    print("This should not execute");
}

main();
