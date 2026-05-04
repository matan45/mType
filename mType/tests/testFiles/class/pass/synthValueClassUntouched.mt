// MYT-274: synthesis must skip value classes (Int/Float/Bool/String wrappers
// have explicit hashCode/equals AND a JIT primitive-protocol fast path).
// This test confirms that boxed primitives in HashMap continue to behave
// correctly after the synthesis pass is enabled.

import * from "../../lib/collections/HashMap.mt";
import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/String.mt";

HashMap<Int, String> map = new HashMap<Int, String>();
map.put(new Int(1), new String("one"));
map.put(new Int(2), new String("two"));
map.put(new Int(3), new String("three"));

print("size: " + map.size());
print("get 1: " + map.get(new Int(1)));
print("get 2: " + map.get(new Int(2)));
print("contains 3: " + map.containsKey(new Int(3)));
print("contains 4: " + map.containsKey(new Int(4)));
