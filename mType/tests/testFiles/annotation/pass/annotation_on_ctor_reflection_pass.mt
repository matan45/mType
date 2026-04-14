// MYT-109 (3a): constructor-annotation reflection native. Before MYT-109 the
// ConstructorDefinition carried an annotations slot but no reflection native
// exposed it. Verifies that hasAnnotation/getAnnotation work on constructors.

import * from "../../lib/reflect/Class.mt";

annotation DefaultCtor {
    bool isDefault = true;
}

class Config {
    @DefaultCtor
    public constructor() { }
}

Class c = Class::forName("Config");
Constructor[] ctors = c.getConstructors();
print(ctors[0].hasAnnotation("DefaultCtor"));
print(ctors[0].hasAnnotation("Missing"));
