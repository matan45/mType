// Test Streams.range() factory - half-open integer range
import * from "../../lib/stream/Stream.mt";
import * from "../../lib/stream/Streams.mt";
import * from "../../lib/primitives/Int.mt";

function main(): void {
    print("Testing Streams.range:");

    // Basic half-open range [0, 5)
    Int[] basic = Streams::range(0, 5).toArray();
    print("Length: " + basic.length);
    for (Int v : basic) {
        print(v);
    }

    // Empty range: start == end
    int emptyCount = Streams::range(5, 5).count();
    print("Empty count: " + emptyCount);

    // Range chained with filter then count
    int evens = Streams::range(0, 10).filter(v -> v.getValue() % 2 == 0).count();
    print("Even count in [0,10): " + evens);

    print("Streams.range test passed!");
}

main();
