class Container<T> {
    T item;

    constructor(T i) {
        this.item = i;
    }

    public function getItem(): T {
        return this.item;
    }
}

class Wrapper<T> {
    Container<T> wrapped;

    constructor(Container<T> c) {
        this.wrapped = c;
    }

    public function unwrap(): Container<T> {
        return this.wrapped;
    }
}
