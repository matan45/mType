import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/String.mt";

// Generic with null safety patterns
class Optional<T> {
    T value;
    Bool present;

    public function Optional() {
        present = false;
    }

    public static function <T> of(T val): Optional<T> {
        Optional<T> opt = new Optional<T>();
        opt.value = val;
        opt.present = true;
        return opt;
    }

    public static function <T> empty(): Optional<T> {
        return new Optional<T>();
    }

    public function isPresent(): Bool {
        return present;
    }

    public function get(): T {
        return value;
    }

    public function orElse(T defaultValue): T {
        if (present) {
            return value;
        }
        return defaultValue;
    }
}

function main(): void {
    Optional<Int> presentInt = Optional.of(new Int(42));
    if (presentInt.isPresent()) {
        print("Present: " + presentInt.get());
    }

    Optional<Int> emptyInt = Optional.empty();
    Int value = emptyInt.orElse(new Int(0));
    print("Default: " + value);

    Optional<String> presentStr = Optional.of(new String("Hello"));
    String str = presentStr.orElse(new String("Default"));
    print("String: " + str);
}

main();
