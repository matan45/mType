// Test: HashMap operations with a null key print an error and return null/false
import * from "../../lib/collections/HashMap.mt";
import * from "../../lib/primitives/Int.mt";

function main(): void {
    HashMap<Int, Int> map = new HashMap<Int, Int>();
    map.put(new Int(1), new Int(100));

    Int putRet = map.put(null, new Int(999));
    print("put returned null: " + (putRet == null));

    Int getRet = map.get(null);
    print("get returned null: " + (getRet == null));

    bool ck = map.containsKey(null);
    print("containsKey: " + ck);

    bool rm = map.remove(null);
    print("remove: " + rm);

    print("size unchanged: " + map.size());
}
main();

// Expected output:
// Error: HashMap.put() - key cannot be null
// put returned null: true
// Error: HashMap.get() - key cannot be null
// get returned null: true
// Error: HashMap.containsKey() - key cannot be null
// containsKey: false
// Error: HashMap.remove() - key cannot be null
// remove: false
// size unchanged: 1
