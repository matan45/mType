// Test: HashMap.putAll() copies entries from another map, with overlaps overwriting
import * from "../../lib/collections/HashMap.mt";
import * from "../../lib/primitives/Int.mt";

function main(): void {
    HashMap<Int, Int> target = new HashMap<Int, Int>();
    target.put(new Int(1), new Int(100));
    target.put(new Int(2), new Int(200));

    HashMap<Int, Int> source = new HashMap<Int, Int>();
    source.put(new Int(2), new Int(999));
    source.put(new Int(3), new Int(300));
    source.put(new Int(4), new Int(400));

    target.putAll(source);

    print("size: " + target.size());
    print("k1: " + target.get(new Int(1)).getValue());
    print("k2 overwritten: " + target.get(new Int(2)).getValue());
    print("k3: " + target.get(new Int(3)).getValue());
    print("k4: " + target.get(new Int(4)).getValue());
}
main();

// Expected output:
// size: 4
// k1: 100
// k2 overwritten: 999
// k3: 300
// k4: 400
