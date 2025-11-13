// Test stream operation chaining
import * from "../../lib/collections/ArrayList.mt";
import * from "../../lib/stream/Stream.mt";
import * from "../../lib/functional/Predicate.mt";
import * from "../../lib/functional/Function.mt";

class EvenPredicate implements Predicate<int> {
    public function test(int value): bool {
        return value % 2 == 0;
    }
}

class SquareFunction implements Function<int, int> {
    public function apply(int value): int {
        return value * value;
    }
}

@Script
function main(): void {
    // Create a list
    ArrayList<int> list = new ArrayList<int>();
    for (int i = 1; i <= 10; i++) {
        list.add(i);
    }

    // Test complex chaining
    print("Testing stream operation chaining:");
    int[] result = list.stream()
        .filter(new EvenPredicate())      // Keep only even: 2,4,6,8,10
        .map<int>(new SquareFunction())   // Square them: 4,16,36,64,100
        .limit(3)                         // Take first 3: 4,16,36
        .toArray();

    print("Result of chained operations:");
    for (int num : result) {
        print(num); // Should print 4, 16, 36
    }

    print("Stream chaining test passed!");
}
