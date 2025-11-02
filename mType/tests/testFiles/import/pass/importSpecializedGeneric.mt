// Test: Import pre-specialized generic class
@Script
import { IntStringPair } from "./modules/SpecializedGenericModule.mt"

var pair = IntStringPair(100, "value");
print(pair.getFirst());
print(pair.getSecond());
