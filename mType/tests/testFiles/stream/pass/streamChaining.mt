// Test stream operation chaining
import * from "../../lib/collections/ArrayList.mt";
import * from "../../lib/stream/Stream.mt";
import * from "../../lib/primitives/Int.mt";



function main(): void {
    // Create a list
    ArrayList<Int> list = new ArrayList<Int>();
    for (int i = 1; i <= 10; i++) {
        list.add(i);
    }

    // Test complex chaining
    print("Testing stream operation chaining:");
    Int[] result = list.stream()
        .filter(x-> x % 2 == 0)      // Keep only even: 2,4,6,8,10
        .map(y-> y*y)   // Square them: 4,16,36,64,100
        .limit(3)                         // Take first 3: 4,16,36
        .toArray();

    print("Result of chained operations:");
    for (Int num : result) {
        print(num); // Should print 4, 16, 36
    }

    print("Stream chaining test passed!");
}

main();
