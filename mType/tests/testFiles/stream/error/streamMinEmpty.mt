// Test: min() on empty stream returns null; calling getValue() on null throws NPE
import * from "../../lib/stream/Stream.mt";
import * from "../../lib/stream/Streams.mt";
import * from "../../lib/functional/Comparator.mt";
import * from "../../lib/primitives/Int.mt";

class IntAscendingComparator implements Comparator<Int> {
    public function compare(Int a, Int b): int {
        return a.getValue() - b.getValue();
    }
}

IntAscendingComparator cmp = new IntAscendingComparator();
Int[] empty = new Int[0];
Int minVal = Streams::of<Int>(empty).min(cmp);
int v = minVal.getValue();
print("should not reach: " + v);
