class GenericPair<T, U> {
    var first: T;
    var second: U;

    constructor(f: T, s: U) {
        this.first = f;
        this.second = s;
    }

    fun getFirst(): T {
        return this.first;
    }

    fun getSecond(): U {
        return this.second;
    }
}

// Pre-specialized version
class IntStringPair extends GenericPair<Int, String> {
    constructor(i: Int, s: String) {
        super(i, s);
    }
}
