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

    function hasData(): bool {
        return data != null;
    }
}

function main(): void {
    Storage<int> intStorage = new Storage<int>();
    intStorage.store(999);
    int value = intStorage.retrieve();
    print("Final value: " + value);
    print("Has data: " + intStorage.hasData());
}

main();