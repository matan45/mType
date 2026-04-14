// MYT-109 (3a): @Retention(SOURCE) annotations are stripped before class
// registration, so reflection no longer sees them at runtime.

import * from "../../lib/reflect/Class.mt";

@Retention(SOURCE)
annotation Draft { }

@Retention(RUNTIME)
annotation Ready { }

@Draft
@Ready
class Demo { }

Class c = Class::forName("Demo");
print(c.hasAnnotation("Draft"));
print(c.hasAnnotation("Ready"));
