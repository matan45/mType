// Test: Import and use ArrayList from lib/collections
import * from "../../lib/primitives/Int.mt";
import { ArrayList } from "../../lib/collections/ArrayList.mt";

ArrayList<Int> ArrayList = new ArrayList<Int>();
ArrayList.add(new Int(10));
ArrayList.add(new Int(20));
ArrayList.add(new Int(30));
print(ArrayList.size());
print(ArrayList.get(1).toString());
