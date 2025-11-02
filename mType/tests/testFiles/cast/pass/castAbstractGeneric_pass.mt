@Script
// Test abstract class with generic parameters
abstract class Container<T> {
    abstract fn get(): T;
    abstract fn set(value: T): void;
}

class Box<T> : Container<T> {
    let value: T;

    fn init(v: T) {
        this.value = v;
    }

    fn get(): T {
        return this.value;
    }

    fn set(v: T): void {
        this.value = v;
    }
}

fn main() {
    let intBox: Box<Int> = new Box<Int>(42);
    let container: Container<Int> = intBox as Container<Int>;  // Upcast with generics

    print(container.get());
    container.set(100);
    print(container.get());

    let backToBox: Box<Int> = container as Box<Int>;  // Downcast with generics
    print(backToBox.get());

    let strBox: Box<String> = new Box<String>("Hello");
    let strContainer: Container<String> = strBox as Container<String>;
    print(strContainer.get());
}
