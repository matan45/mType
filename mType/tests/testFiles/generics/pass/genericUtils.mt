// Generic utility class for import testing
class Wrapper<T> {
    T data;

    function wrap(T item): void {
        data = item;
    }

    function unwrap(): T {
        return data;
    }

    function toString(): string {
        return "Wrapped: " + data;
    }
}