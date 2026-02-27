// Test Stack serialization and deserialization roundtrip

import * from "../../lib/json/Json.mt";
import * from "../../lib/collections/Stack.mt";
import * from "../../lib/primitives/Int.mt";

Stack<Int> stack = new Stack<Int>();
stack.push(new Int(1));
stack.push(new Int(2));
stack.push(new Int(3));

string json = Json::serialize(stack);
print(json);

// Deserialize and verify
Stack<Int> restored = Json::deserializeAs(json, "Stack");
print(restored.size());
print(restored.pop().getValue());
print(restored.pop().getValue());
print(restored.pop().getValue());

print("Test passed");
