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
    print(Base.convert(new Int(1)));
    print(Derived.convert(new Int(2)));
    print(Base.convert(new String("base")));
    print(Derived.convert(new String("derived")));
}

main();
