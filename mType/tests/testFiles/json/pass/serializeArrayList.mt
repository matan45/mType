// Test ArrayList serialization and deserialization roundtrip

import * from "../../lib/json/Json.mt";
import * from "../../lib/collections/ArrayList.mt";
import * from "../../lib/primitives/Int.mt";

ArrayList<Int> list = new ArrayList<Int>();
list.add(new Int(10));
list.add(new Int(20));
list.add(new Int(30));

string json = Json::serialize(list);
print(json);

// Deserialize and verify
ArrayList<Int> restored = Json::deserializeAs(json, "ArrayList");
print(restored.size());
print(restored.get(0).getValue());
print(restored.get(1).getValue());
print(restored.get(2).getValue());

print("Test passed");
