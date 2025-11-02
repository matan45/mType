import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/String.mt";

// Lambda type inference with generics
class Transformer<T, R> {
    function(T): R transform;

    public function setTransform(function(T): R fn): void {
        transform = fn;
    }

    public function apply(T input): R {
        return transform(input);
    }
}

function main(): void {
    Transformer<Int, String> intToStr = new Transformer<Int, String>();
    intToStr.setTransform(function(Int x): String {
        return new String("Number: " + x);
    });

    String result = intToStr.apply(new Int(42));
    print(result);
}

main();
