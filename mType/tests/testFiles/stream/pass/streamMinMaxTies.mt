// Test min and max on stream where all elements are equal
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
    print("Testing stream min/max ties:");

    IntAscendingComparator cmp = new IntAscendingComparator();

    Int[] data1 = [new Int(5), new Int(5), new Int(5)];
    Int minVal = Streams::of<Int>(data1).min(cmp);
    print("min: " + minVal.getValue());

    Int[] data2 = [new Int(5), new Int(5), new Int(5)];
    Int maxVal = Streams::of<Int>(data2).max(cmp);
    print("max: " + maxVal.getValue());

    print("Stream min/max ties test passed!");
}
main();
