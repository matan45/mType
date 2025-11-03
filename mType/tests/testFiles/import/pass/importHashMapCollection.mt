// Test: Import and use HashMap from lib/collections
import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/String.mt";
import { HashMap } from "../../lib/collections/HashMap.mt";

HashMap<String, Int> map = new HashMap<String, Int>();
map.put(new String("one"), new Int(1));
map.put(new String("two"), new Int(2));
map.put(new String("three"), new Int(3));
print(map.size());
print(map.get(new String("two")).toString());
