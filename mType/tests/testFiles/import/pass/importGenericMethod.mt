// Test: Import class with generic methods
import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/String.mt";
import { Processor } from "modules/GenericMethodModule.mt";

Processor processor = new Processor();

Int intResult = processor.process<Int>(42);
String stringResult = processor.process<String>("test");

print(intResult);
print(stringResult);
