// Test stream min and max with comparator on non-empty stream
import * from "../../lib/stream/Stream.mt";
import * from "../../lib/stream/Streams.mt";
import * from "../../lib/functional/Comparator.mt";
import * from "../../lib/primitives/Int.mt";

class IntAscendingComparator implements Comparator<Int> {
    public function compare(Int a, Int b): int {
        return a.getValue() - b.getValue();
    }
}

function main(): void {
    print("Testing stream min/max:");

    IntAscendingComparator cmp = new IntAscendingComparator();

    Int[] data1 = [new Int(3), new Int(1), new Int(4), new Int(1), new Int(5), new Int(9), new Int(2), new Int(6)];
    Int minVal = Streams::of<Int>(data1).min(cmp);
    print("min: " + minVal.getValue());

    Int[] data2 = [new Int(3), new Int(1), new Int(4), new Int(1), new Int(5), new Int(9), new Int(2), new Int(6)];
    Int maxVal = Streams::of<Int>(data2).max(cmp);
    print("max: " + maxVal.getValue());

    print("Stream min/max basic test passed!");
}
main();
