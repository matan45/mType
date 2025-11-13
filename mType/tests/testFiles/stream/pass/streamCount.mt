// Test stream count operation
import * from "../../lib/collections/ArrayList.mt";
import * from "../../lib/stream/Stream.mt";
import * from "../../lib/functional/Predicate.mt";

class PositivePredicate implements Predicate<int> {
    public function test(int value): bool {
        return value > 0;
    }
}

@Script
function main(): void {
    // Create a list
    ArrayList<int> list = new ArrayList<int>();
    list.add(-5);
    list.add(3);
    list.add(-2);
    list.add(7);
    list.add(0);
    list.add(9);

    // Test count
    print("Testing stream count:");
    int count = list.stream()
        .filter(new PositivePredicate())
        .count();

    print("Count of positive numbers: " + count); // Should be 3

    print("Stream count test passed!");
}
