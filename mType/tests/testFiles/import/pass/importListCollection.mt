// Test: Import and use List from lib/collections
import * from "../../lib/primitives/Int.mt";
import { List } from "../../lib/collections/List.mt";

List<Int> list = new List<Int>();
list.add(new Int(10));
list.add(new Int(20));
list.add(new Int(30));
print(list.size());
print(list.get(1).toString());
