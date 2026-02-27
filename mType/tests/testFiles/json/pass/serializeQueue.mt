// Test ArrayQueue serialization and deserialization roundtrip

import * from "../../lib/json/Json.mt";
import * from "../../lib/collections/ArrayQueue.mt";
import * from "../../lib/primitives/Int.mt";

ArrayQueue<Int> queue = new ArrayQueue<Int>();
queue.enqueue(new Int(10));
queue.enqueue(new Int(20));
queue.enqueue(new Int(30));

string json = Json::serialize(queue);
print(json);

// Deserialize and verify FIFO order
ArrayQueue<Int> restored = Json::deserializeAs(json, "ArrayQueue");
print(restored.size());
print(restored.dequeue().getValue());
print(restored.dequeue().getValue());
print(restored.dequeue().getValue());

print("Test passed");
