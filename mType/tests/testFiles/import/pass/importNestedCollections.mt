// Test: Using nested generic collections - ArrayList<HashMap<String,Int>>
import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/String.mt";
import { ArrayList } from "../../lib/collections/ArrayList.mt";
import { HashMap } from "../../lib/collections/HashMap.mt";

ArrayList<HashMap<String, Int>> ArrayListOfMaps = new ArrayList<HashMap<String, Int>>();
HashMap<String, Int> map1 = new HashMap<String, Int>();
map1.put(new String("a"), new Int(1));
HashMap<String, Int> map2 = new HashMap<String, Int>();
map2.put(new String("b"), new Int(2));

ArrayListOfMaps.add(map1);
ArrayListOfMaps.add(map2);
print(ArrayListOfMaps.size());
