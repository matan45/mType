// Test: Import and use List from lib/collections
@Script
import { List } from "../../../../../../lib/collections/List.mt"

var list = List<Int>();
list.add(10);
list.add(20);
list.add(30);
print(list.size());
print(list.get(1));
