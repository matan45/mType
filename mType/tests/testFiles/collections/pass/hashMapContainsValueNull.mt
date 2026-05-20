// Test: HashMap.containsValue(null) returns true when a null value is stored
import * from "../../lib/collections/HashMap.mt";
import * from "../../lib/primitives/Int.mt";

function main(): void {
    HashMap<Int, Int> map = new HashMap<Int, Int>();
    map.put(new Int(1), new Int(100));
    map.put(new Int(2), null);
    map.put(new Int(3), new Int(300));

    print("contains null: " + map.containsValue(null));
    print("contains 100: " + map.containsValue(new Int(100)));

    HashMap<Int, Int> mapNoNulls = new HashMap<Int, Int>();
    mapNoNulls.put(new Int(1), new Int(1));
    print("noNulls contains null: " + mapNoNulls.containsValue(null));
}
main();

// Expected output:
// contains null: true
// contains 100: true
// noNulls contains null: false
