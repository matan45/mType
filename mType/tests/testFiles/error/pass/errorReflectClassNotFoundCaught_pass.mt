// Test: Class::forName for a missing class throws, but a try/catch
// recovers and a subsequent valid forName still works.
import * from "../../lib/exceptions/Exception.mt";
import * from "../../lib/reflect/Class.mt";

function main(): void {
    try {
        Class c = Class::forName("DefinitelyNotARealClass");
        print("nope " + c.getName());
    } catch (Exception e) {
        print("caught reflection lookup");
    }
    Class real = Class::forName("Exception");
    print("real: " + real.getName());
}
main();
