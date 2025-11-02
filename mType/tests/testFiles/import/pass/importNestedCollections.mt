// Test: Using nested generic collections - List<HashMap<String,Int>>
@Script
import { List } from "../../../../../../lib/collections/List.mt"
import { HashMap } from "../../../../../../lib/collections/HashMap.mt"

var listOfMaps = List<HashMap<String, Int>>();
var map1 = HashMap<String, Int>();
map1.put("a", 1);
var map2 = HashMap<String, Int>();
map2.put("b", 2);

listOfMaps.add(map1);
listOfMaps.add(map2);
print(listOfMaps.size());
