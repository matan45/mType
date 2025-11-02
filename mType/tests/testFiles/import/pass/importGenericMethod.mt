// Test: Import class with generic methods
@Script
import { Processor } from "./modules/GenericMethodModule.mt"

var processor = Processor();

var intResult = processor.process<Int>(42);
var stringResult = processor.process<String>("test");

print(intResult);
print(stringResult);
