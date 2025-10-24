// Test type parameter shadowing error
// Class<T> has method <T> which shadows the class type parameter

class Container<T> {
    T value;

    // ERROR: This method's <T> shadows the class's <T>
    public function <T> transform(T input): T {
        return input;
    }
}

function main(): void {
    Container<int> container = new Container<int>();
    // This should trigger an error due to type parameter shadowing
}

main();
