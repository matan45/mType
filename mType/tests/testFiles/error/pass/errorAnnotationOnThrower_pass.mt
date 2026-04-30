// Test: a user-declared annotation placed on a throwing method is
// still readable via reflection from inside the catch handler.
import * from "../../lib/exceptions/Exception.mt";
import * from "../../lib/reflect/Class.mt";
import * from "../../lib/reflect/Method.mt";

annotation DocThrows {
    string what = "Exception";
}

class Repo {
    @DocThrows(what = "DataException")
    public function load(): void {
        throw new Exception("bad data");
    }
}

function main(): void {
    Repo r = new Repo();
    try {
        r.load();
    } catch (Exception e) {
        print("caught: " + e.getMessage());
        Class c = Class::forName("Repo");
        Method m = c.getDeclaredMethod("load", 0);
        print(m.hasAnnotation("DocThrows"));
    }
}
main();
