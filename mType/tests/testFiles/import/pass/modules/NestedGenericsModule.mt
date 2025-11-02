class Container<T> {
    var item: T;

    constructor(i: T) {
        this.item = i;
    }

    fun getItem(): T {
        return this.item;
    }
}

class Wrapper<T> {
    var wrapped: Container<T>;

    constructor(c: Container<T>) {
        this.wrapped = c;
    }

    fun unwrap(): Container<T> {
        return this.wrapped;
    }
}
