import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/String.mt";

// Generic functional interfaces with lambdas
class Function<T, R> {
    function(T): R fn;

    public function setFunction(function(T): R f): void {
        fn = f;
    }

    public function apply(T input): R {
        return fn(input);
    }
}

function main(): void {
    Function<Int, String> intToString = new Function<Int, String>();
    intToString.setFunction(function(Int x): String {
        return new String("Value: " + x);
    });

    String result = intToString.apply(new Int(42));
    print(result);

    Function<String, Int> stringLength = new Function<String, Int>();
    stringLength.setFunction(function(String s): Int {
        return new Int(10);
    });

    Int len = stringLength.apply(new String("test"));
    print("Length: " + len);
}

main();
