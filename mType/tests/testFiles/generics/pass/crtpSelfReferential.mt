import * from "../../lib/primitives/Int.mt";

// CRTP-style self-referential bound: class Base<T extends Base<T>>.
// A concrete subclass Sub extends Base<Sub> and uses a self-typed method
// returning T (i.e. Sub) so chaining is type-correct.
class Base<T extends Base<T>> {
    int counter;

    public constructor() {
        this.counter = 0;
    }

    public function increment(): T {
        this.counter = this.counter + 1;
        // Return self typed as T (the CRTP self-type).
        return this as T;
    }

    public function getCounter(): int {
        return this.counter;
    }
}

class Sub extends Base<Sub> {
    string label;

    public constructor(string name) {
        super();
        this.label = name;
    }

    public function getLabel(): string {
        return this.label;
    }
}

function main(): void {
    print("CRTP self-referential test");

    Sub a = new Sub("alpha");
    Sub b = new Sub("beta");

    // increment() returns Sub (the CRTP self-type), so chaining is valid.
    Sub chained = a.increment().increment().increment();
    print("alpha counter: " + chained.getCounter());
    print("alpha label: " + chained.getLabel());

    b.increment();
    print("beta counter: " + b.getCounter());
    print("beta label: " + b.getLabel());

    print("CRTP test completed");
}

main();
