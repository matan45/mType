import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/String.mt";
import * from "../../lib/primitives/Bool.mt";

// Generic with null safety patterns
class Optional<T> {
    T value;
    Bool present;

    public constructor() {
        present = new Bool(false);
    }

    public static function <U> of(U val): Optional<U> {
        Optional<U> opt = new Optional<U>();
        opt.value = val;
        opt.present = new Bool(true);
        return opt;
    }

    public static function <U> empty(): Optional<U> {
        return new Optional<U>();
    }

    public function isPresent(): Bool {
        return present;
    }

    public function get(): T {
        return value;
    }

    public function orElse(T defaultValue): T {
        if (present.getValue()) {
            return value;
        }
        return defaultValue;
    }
}

function main(): void {
    Optional<Int> presentInt = Optional::of<Int>(new Int(42));
    if (presentInt.isPresent().getValue()) {
        print("Present: " + presentInt.get().toString());
    }

    Optional<Int> emptyInt = Optional::empty<Int>();
    Int value = emptyInt.orElse(new Int(0));
    print("Default: " + value.toString());

    Optional<String> presentStr = Optional::of<String>(new String("Hello"));
    String str = presentStr.orElse(new String("Default"));
    print("String: " + str.toString());
}

main();
