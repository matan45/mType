// Test: HashMap.containsValue() true/false paths
import * from "../../lib/collections/HashMap.mt";
import * from "../../lib/primitives/Int.mt";

function main(): void {
    HashMap<Int, Int> map = new HashMap<Int, Int>();
    map.put(new Int(1), new Int(100));
    map.put(new Int(2), new Int(200));
    map.put(new Int(3), new Int(300));

    print("contains 100: " + map.containsValue(new Int(100)));
    print("contains 200: " + map.containsValue(new Int(200)));
    print("contains 999: " + map.containsValue(new Int(999)));
}
main();

// Expected output:
// contains 100: true
// contains 200: true
// contains 999: false
