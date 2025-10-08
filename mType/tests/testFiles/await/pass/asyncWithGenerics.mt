// Test async functions with generic type parameters

class Box<T> {
    T item;

    public constructor(T i) {
        this.item = i;
    }

    public function getItem(): T {
        return this.item;
    }
}

class Data {
    int num;

    public constructor(int n) {
        this.num = n;
    }

    public function getNum(): int {
        return this.num;
    }
}

print("=== Async With Generics Test ===");

// Generic async function
function async <T> wrap(T value): Promise<Box<T>> {
    Box<T> box = new Box<T>(value);
    return box;
}

// Main function to run test
function async main(): Promise<Box<Data>> {
    // Test with object type
    Data d = new Data(42);
    Box<Data> box = await wrap<Data>(d);
    Data unwrapped = box.getItem();
    print("Wrapped data: " + unwrapped.getNum());
    return box;
}

main();
