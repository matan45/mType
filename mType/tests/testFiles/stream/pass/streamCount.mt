// Test stream count operation
import * from "../../lib/collections/ArrayList.mt";
import * from "../../lib/stream/Stream.mt";
import * from "../../lib/functional/Predicate.mt";
import * from "../../lib/primitives/Int.mt";

// MYT-360 — primitives can't be generic type arguments; was Predicate<int>.
class PositivePredicate implements Predicate<Int> {
    public function test(Int value): bool {
        return value.getValue() > 0;
    }
}


function main(): void {
    // Create a list
    ArrayList<Int> list = new ArrayList<Int>();
    list.add(-5);
    list.add(3);
    list.add(-2);
    list.add(7);
    list.add(0);
    list.add(9);

    // Test count
    print("Testing stream count:");
    Int count = list.stream()
        .filter(x-> x>0)
        .count();

    print("Count of positive numbers: " + count); // Should be 3

    print("Stream count test passed!");
}
main();
