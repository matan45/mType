import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/Bool.mt";

// Generic class with method overloading
class Storage<T> {
    T data;

    public function store(T value): void {
        data = value;
        print("Stored: " + value);
    }

    public function retrieve(): T {
        print("Retrieved: " + data);
        return data;
    }

    public function hasData(): Bool {
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