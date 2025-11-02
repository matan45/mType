// Test: Import generic interface and implementation
@Script
import { Collection, SimpleCollection } from "./modules/GenericInterfaceModule.mt"

var collection: Collection<String> = SimpleCollection<String>();
collection.add("first");
collection.add("second");
collection.add("third");

print(collection.size());
print(collection.get(1));
