import * from "../../../lib/primitives/Int.mt";
import * from "../../../lib/primitives/String.mt";
class GenericPair<T, U> {
    T first;
    U second;

    constructor(T f, U s) {
        this.first = f;
        this.second = s;
    }

    public function getFirst(): T {
        return this.first;
    }

    public function getSecond(): U {
        return this.second;
    }
}

// Pre-specialized version
class IntStringPair extends GenericPair<Int, String> {
    constructor(Int i,String s): super(i, s) {
    }
}
