import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/String.mt";

// Constraints with wildcard-like behavior
interface Bounded {
    function getUpperBound(): Int;
    function getLowerBound(): Int;
}

class Range extends Bounded {
    Int lower;
    Int upper;

    public function Range(Int l, Int u) {
        lower = l;
        upper = u;
    }

    public function getUpperBound(): Int {
        return upper;
    }

    public function getLowerBound(): Int {
        return lower;
    }
}

class BoundedContainer<T extends Bounded> {
    T range;

    public function setRange(T r): void {
        range = r;
    }

    public function printBounds(): void {
        print("Lower: " + range.getLowerBound());
        print("Upper: " + range.getUpperBound());
    }
}

function main(): void {
    BoundedContainer<Range> container = new BoundedContainer<Range>();
    container.setRange(new Range(new Int(0), new Int(100)));
    container.printBounds();
}

main();
