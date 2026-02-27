// Test HashMap serialization and deserialization roundtrip

import * from "../../lib/json/Json.mt";
import * from "../../lib/collections/HashMap.mt";
import * from "../../lib/primitives/String.mt";
import * from "../../lib/primitives/Int.mt";

HashMap<String, Int> map = new HashMap<String, Int>();
map.put(new String("alice"), new Int(30));
map.put(new String("bob"), new Int(25));

string json = Json::serialize(map);
print(json);

// Deserialize and verify get() works
HashMap<String, Int> restored = Json::deserializeAs(json, "HashMap");
print(restored.size());
print(restored.get(new String("alice")).getValue());
print(restored.get(new String("bob")).getValue());

print("Test passed");
