// MYT-42: calling .getClass() on a null receiver must produce a clean
// runtime error, not crash. Registered as an ERROR_EXPECTED test — any
// thrown exception counts as a pass.

import * from "../../lib/reflect/Class.mt";

class Widget {
    public constructor() {}
}

Widget w = null;
Class c = w.getClass();
print(c.getName());
