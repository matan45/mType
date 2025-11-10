import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/String.mt";

// Generic functional interfaces with lambdas
interface Func<T, R> {
    function apply(T input): R;
}

class FunctionHolder<T, R> {
    Func<T, R> fn;

    public function setFunction(Func<T, R> f): void {
        fn = f;
    }

    public function apply(T input): R {
        return fn.apply(input);
    }
}

function main(): void {
    FunctionHolder<Int, String> intToString = new FunctionHolder<Int, String>();
    intToString.setFunction(x -> new String("Value: " + x));

    String result = intToString.apply(new Int(42));
    print(result.toString());

    FunctionHolder<String, Int> stringLength = new FunctionHolder<String, Int>();
    stringLength.setFunction(s -> new Int(10));

    Int len = stringLength.apply(new String("test"));
    print("Length: " + len.toString());
}

main();
