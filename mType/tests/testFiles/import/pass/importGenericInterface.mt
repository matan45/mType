// Test: Import generic interface and implementation

import * from "../../lib/primitives/String.mt";
import { Collection, SimpleCollection } from "modules/GenericInterfaceModule.mt";

Collection<String> collection = new SimpleCollection<String>();
collection.add("first");
collection.add("second");
collection.add("third");

print(collection.size());
print(collection.get(1).toString());
