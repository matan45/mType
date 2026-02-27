// Test LinkedList serialization and deserialization roundtrip

import * from "../../lib/json/Json.mt";
import * from "../../lib/collections/LinkedList.mt";
import * from "../../lib/primitives/Int.mt";

LinkedList<Int> list = new LinkedList<Int>();
list.add(new Int(100));
list.add(new Int(200));
list.add(new Int(300));

string json = Json::serialize(list);
print(json);

// Deserialize and verify
LinkedList<Int> restored = Json::deserializeAs(json, "LinkedList");
print(restored.size());
print(restored.get(0).getValue());
print(restored.get(1).getValue());
print(restored.get(2).getValue());

print("Test passed");
