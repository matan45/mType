// Test: Import multiple collections
import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/String.mt";
import { List } from "../../lib/collections/List.mt";
import { HashMap } from "../../lib/collections/HashMap.mt";
import { HashSet } from "../../lib/collections/HashSet.mt";
import { Stack } from "../../lib/collections/Stack.mt";
import { Queue } from "../../lib/collections/Queue.mt";

List<Int> list = new List<Int>();
HashMap<String, Int> map = new HashMap<String, Int>();
HashSet<String> set = new HashSet<String>();
Stack<Int> stack = new Stack<Int>();
Queue<Int> queue = new Queue<Int>();

list.add(new Int(1));
map.put(new String("key"), new Int(1));
set.add(new String("value"));
stack.push(new Int(1));
queue.enqueue(new Int(1));

print("All collections imported successfully");
