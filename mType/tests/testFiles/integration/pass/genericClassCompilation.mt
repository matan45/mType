// Test generic class compilation/serialization
class Container<T> {
    T data;

    function store(T value): void {
        data = value;
    }

    function retrieve(): T {
        return data;
    }

    function hasData(): bool {
        return data != null;
    }
}

function main(): void {
    print("Generic class compilation test");
}

main();