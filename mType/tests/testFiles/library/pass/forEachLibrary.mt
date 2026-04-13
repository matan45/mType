import * from "../../lib/primitives/String.mt";
import * from "../../lib/collections/ArrayList.mt";

ArrayList<String> names = new ArrayList<String>();
names.add("Alice");
names.add("Bob");
names.add("Charlie");

for (String name : names) {
    print(name);
}
