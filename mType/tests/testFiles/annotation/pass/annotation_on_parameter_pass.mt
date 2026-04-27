// MYT-110: annotation applied to a method parameter is readable via reflection.
// Exercises: parse, validator (PARAMETER target host), AST -> runtime copy,
// Parameter.getAnnotation -> native -> MethodDefinition::getParameterAnnotation.

import * from "../../lib/reflect/Class.mt";
import * from "../../lib/reflect/Method.mt";
import * from "../../lib/reflect/Parameter.mt";
import * from "../../lib/reflect/Annotation.mt";

annotation NotNull { }

annotation Email {
    string pattern = ".+@.+";
}

class UserService {
    public function createUser(@NotNull string name, @Email string email): void {
    }
}

Class c = Class::forName("UserService");
Method m = c.getDeclaredMethod("createUser", 2);
Parameter[] params = m.getParameters();

// MYT-214: getParameters() returns user-declared parameters only (no `this`).
// name is at index 0, email at index 1.
print(params[0].getName());
print(params[0].hasAnnotation("NotNull"));
print(params[1].hasAnnotation("Email"));

Annotation? a = params[1].getAnnotation("Email");
if (a != null) {
    print(a.getString("pattern"));
}
