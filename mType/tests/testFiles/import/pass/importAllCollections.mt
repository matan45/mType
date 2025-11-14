// Test: Import multiple collections
import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/String.mt";
import { ArrayList } from "../../lib/collections/ArrayList.mt";
import { HashMap } from "../../lib/collections/HashMap.mt";
import { HashSet } from "../../lib/collections/HashSet.mt";
import { Stack } from "../../lib/collections/Stack.mt";
import { ArrayQueue } from "../../lib/collections/ArrayQueue.mt";

ArrayList<Int> ArrayList = new ArrayList<Int>();
HashMap<String, Int> map = new HashMap<String, Int>();
HashSet<String> set = new HashSet<String>();
Stack<Int> stack = new Stack<Int>();
ArrayQueue<Int> ArrayQueue = new ArrayQueue<Int>();

ArrayList.add(new Int(1));
map.put(new String("key"), new Int(1));
set.add(new String("value"));
stack.push(new Int(1));
ArrayQueue.enArrayQueue(new Int(1));

print("All collections imported successfully");
