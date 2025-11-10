class GenericBox<T> {
    T value;

    constructor(T val) {
        this.value = val;
    }

    public function getValue(): T {
        return this.value;
    }

    public function setValue(T val): void {
        this.value = val;
    }
}
