// MYT-110: annotation applied to a constructor parameter is readable via
// reflection. Mirrors annotation_on_parameter_pass.mt on the ctor path.

import * from "../../lib/reflect/Class.mt";
import * from "../../lib/reflect/Parameter.mt";
import * from "../../lib/reflect/Annotation.mt";

annotation Inject { }

class Service {
    public constructor(@Inject string dbUrl) {
        return;
    }
}

Class c = Class::forName("Service");
Constructor[] ctors = c.getConstructors();
Parameter[] params = ctors[0].getParameters();

print(params[0].getName());
print(params[0].hasAnnotation("Inject"));
print(params[0].hasAnnotation("Missing"));
