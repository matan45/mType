// Test: Import generic class and use with different types
import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/String.mt";
import { GenericBox } from "modules/GenericBoxModule.mt";

GenericBox<Int> intBox = new GenericBox<Int>(new Int(42));
GenericBox<String> stringBox = new GenericBox<String>(new String("Hello"));

print(intBox.getValue().toString());
print(stringBox.getValue().toString());
