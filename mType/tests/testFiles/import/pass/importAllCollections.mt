// Test: Import multiple collections
@Script
import { List } from "../../../../../../lib/collections/List.mt"
import { HashMap } from "../../../../../../lib/collections/HashMap.mt"
import { HashSet } from "../../../../../../lib/collections/HashSet.mt"
import { Stack } from "../../../../../../lib/collections/Stack.mt"
import { Queue } from "../../../../../../lib/collections/Queue.mt"

var list = List<Int>();
var map = HashMap<String, Int>();
var set = HashSet<String>();
var stack = Stack<Int>();
var queue = Queue<Int>();

list.add(1);
map.put("key", 1);
set.add("value");
stack.push(1);
queue.enqueue(1);

print("All collections imported successfully");
