// MathLib: Generic container for cross-library generics testing

class Container<T> {
    private T value;

    public constructor(T value) {
        this.value = value;
    }

    public function getValue(): T {
        return this.value;
    }

    public function setValue(T newValue): void {
        this.value = newValue;
    }

    public function toString(): string {
        return "Container(" + this.value + ")";
    }
}
