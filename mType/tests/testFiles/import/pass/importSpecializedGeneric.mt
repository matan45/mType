// Test: Import pre-specialized generic class

import { IntStringPair } from "modules/SpecializedGenericModule.mt";

IntStringPair pair = new IntStringPair(100, "value");
print(pair.getFirst().toString());
print(pair.getSecond().toString());
