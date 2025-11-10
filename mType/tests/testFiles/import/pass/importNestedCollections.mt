// Test: Using nested generic collections - List<HashMap<String,Int>>
import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/String.mt";
import { List } from "../../lib/collections/List.mt";
import { HashMap } from "../../lib/collections/HashMap.mt";

List<HashMap<String, Int>> listOfMaps = new List<HashMap<String, Int>>();
HashMap<String, Int> map1 = new HashMap<String, Int>();
map1.put(new String("a"), new Int(1));
HashMap<String, Int> map2 = new HashMap<String, Int>();
map2.put(new String("b"), new Int(2));

listOfMaps.add(map1);
listOfMaps.add(map2);
print(listOfMaps.size());
