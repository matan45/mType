// Test: Import and use HashMap from lib/collections
@Script
import { HashMap } from "../../../../../../lib/collections/HashMap.mt"

var map = HashMap<String, Int>();
map.put("one", 1);
map.put("two", 2);
map.put("three", 3);
print(map.size());
print(map.get("two"));
