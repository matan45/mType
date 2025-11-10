// Test generic class compilation/serialization
class Container<T> {
    T data;

    public function store(T value): void {
        data = value;
    }

    public function retrieve(): T {
        return data;
    }

    public function hasData(): bool {
        return data != null;
    }
}

function main(): void {
    print("Generic class compilation test");
}

main();