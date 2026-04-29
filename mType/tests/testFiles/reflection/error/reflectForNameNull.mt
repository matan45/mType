// Test: Class::forName(null) must reject — null is not a valid class name.

import * from "../../lib/reflect/Class.mt";

string s = null;
Class c = Class::forName(s);

print("Should not reach here");
