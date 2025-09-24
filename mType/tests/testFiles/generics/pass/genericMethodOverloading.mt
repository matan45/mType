import "../../lib/primitives/Int.mt";
import "../../lib/primitives/Bool.mt";

// Generic class with method overloading
class Storage<T> {
    T data;

    function store(T value): void {
        data = value;
        print("Stored: " + value);
    }

    function retrieve(): T {
        print("Retrieved: " + data);
        return data;
    }

    function hasData(): Bool {
        return new Bool(data != null);
    }
}

function main(): void {
    Storage<Int> intStorage = new Storage<Int>();
    intStorage.store(new Int(999));
    Int value = intStorage.retrieve();
    print("Final value: " + value);
    print("Has data: " + intStorage.hasData());
}

main();