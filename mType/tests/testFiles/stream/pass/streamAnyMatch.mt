// Test stream anyMatch operation
import * from "../../lib/collections/ArrayList.mt";
import * from "../../lib/stream/Stream.mt";
import * from "../../lib/functional/Predicate.mt";

class GreaterThan10Predicate implements Predicate<int> {
    public function test(int value): bool {
        return value > 10;
    }
}

@Script
function main(): void {
    // Create a list
    ArrayList<int> list = new ArrayList<int>();
    list.add(1);
    list.add(5);
    list.add(15);
    list.add(3);

    // Test anyMatch
    print("Testing stream anyMatch:");
    bool hasLargeNumber = list.stream().anyMatch(new GreaterThan10Predicate());

    if (hasLargeNumber) {
        print("Found at least one number > 10");
    }

    print("Stream anyMatch test passed!");
}
