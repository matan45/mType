import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/String.mt";

// Static generic method shadowing
class Base {
    public static function <T> convert(T value): String {
        return new String("Base: " + value);
    }
}

class Derived extends Base {
    public static function <T> convert(T value): String {
        return new String("Derived: " + value);
    }
}

function main(): void {
    String r1 = Base::convert<Int>(new Int(1));
    print(r1.toString());
    String r2 = Derived::convert<Int>(new Int(2));
    print(r2.toString());
    String r3 = Base::convert<String>(new String("base"));
    print(r3.toString());
    String r4 = Derived::convert<String>(new String("derived"));
    print(r4.toString());
}

main();
