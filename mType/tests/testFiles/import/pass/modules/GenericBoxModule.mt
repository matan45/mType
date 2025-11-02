class GenericBox<T> {
    var value: T;

    constructor(val: T) {
        this.value = val;
    }

    fun getValue(): T {
        return this.value;
    }

    fun setValue(val: T): Void {
        this.value = val;
    }
}
