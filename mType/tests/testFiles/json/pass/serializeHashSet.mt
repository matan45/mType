// Test HashSet serialization and deserialization roundtrip

import * from "../../lib/json/Json.mt";
import * from "../../lib/collections/HashSet.mt";
import * from "../../lib/primitives/String.mt";

HashSet<String> set = new HashSet<String>();
set.add(new String("apple"));
set.add(new String("banana"));
set.add(new String("cherry"));

string json = Json::serialize(set);
print(json);

// Deserialize and verify contains() works
HashSet<String> restored = Json::deserializeAs(json, "HashSet");
print(restored.size());
print(restored.contains(new String("apple")));
print(restored.contains(new String("banana")));
print(restored.contains(new String("cherry")));

print("Test passed");
