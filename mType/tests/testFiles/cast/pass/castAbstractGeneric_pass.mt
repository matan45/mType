import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/String.mt";
// Test abstract class with generic parameters
abstract class Container<T> {
    public abstract function get(): T;
    public abstract function set(T value): void;
}

class Box<T> extends Container<T> {
    private T value;

    constructor(T v){
        this.value = v;
    }

    public function get(): T {
        return this.value;
    }

    public function set(T v): void {
        this.value = v;
    }
}


function main(): void {
    Box<Int> intBox = new Box<Int>(new Int(42));
    Container<Int> container = (Container<Int>)intBox;  // Upcast with generics

    print(container.get().getValue());
    container.set(new Int(100));
    print(container.get().getValue());

    Box<Int> backToBox = (Box<Int>)container;  // Downcast with generics
    print(backToBox.get().getValue());

    Box<String> strBox = new Box<String>(new String("Hello"));
    Container<String> strContainer = strBox;
    print(strContainer.get().getValue());
}
main();