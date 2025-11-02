// Test: Import generic class and use with different types
@Script
import { GenericBox } from "./modules/GenericBoxModule.mt"

var intBox = GenericBox<Int>(42);
var stringBox = GenericBox<String>("Hello");

print(intBox.getValue());
print(stringBox.getValue());
