// Generic utility class for import testing
class Wrapper<T> {
    T data;

    public function wrap(T item): void {
        data = item;
    }

    public function unwrap(): T {
        return data;
    }

    public function toString(): string {
        return "Wrapped: " + data;
    }
}