import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/String.mt";

// Generic class wrapping a typed array. Verify that an array of length 0
// reports length 0, foreach body never executes, and the wrapper continues
// to operate normally.
class Holder<T> {
    T[] data;

    public constructor(T[] init) {
        this.data = init;
    }

    public function size(): int {
        return this.data.length;
    }
}

function main(): void {
    print("Empty generic array test");

    // Empty Int array.
    Int[] noInts = new Int[0];
    Holder<Int> intHolder = new Holder<Int>(noInts);
    print("Int holder length: " + intHolder.size());

    int intIters = 0;
    for (Int v : noInts) {
        intIters = intIters + 1;
        print("Should not print int");
    }
    print("Int iterations: " + intIters);

    // Empty String array.
    String[] noStrings = new String[0];
    Holder<String> strHolder = new Holder<String>(noStrings);
    print("String holder length: " + strHolder.size());

    int strIters = 0;
    for (String s : noStrings) {
        strIters = strIters + 1;
        print("Should not print string");
    }
    print("String iterations: " + strIters);

    print("Empty generic array test completed");
}

main();
