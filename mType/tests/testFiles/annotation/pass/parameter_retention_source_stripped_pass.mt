// MYT-110: @Retention(SOURCE) stripping applies to parameter annotations too.
// The Draft annotation must not survive into runtime reflection; Keep must.

import * from "../../lib/reflect/Class.mt";
import * from "../../lib/reflect/Method.mt";
import * from "../../lib/reflect/Parameter.mt";

@Retention(SOURCE)
annotation Draft { }

@Retention(RUNTIME)
annotation Keep { }

class Demo {
    public function foo(@Draft @Keep int x): void {
        return;
    }
}

Class c = Class::forName("Demo");
Method m = c.getDeclaredMethod("foo", 1);
Parameter[] params = m.getParameters();

// Index 0 is `this`, index 1 is x.
print(params[1].hasAnnotation("Draft"));
print(params[1].hasAnnotation("Keep"));
