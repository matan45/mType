import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/String.mt";

// Lambda type inference with generics
interface TransformerFunction<T, R> {
    function apply(T input): R;
}

class Transformer<T, R> {
    TransformerFunction<T, R> transformFunc;

    public function setTransform(TransformerFunction<T, R> fn): void {
        transformFunc = fn;
    }

    public function apply(T input): R {
        return transformFunc.apply(input);
    }
}

function main(): void {
    Transformer<Int, String> intToStr = new Transformer<Int, String>();
    intToStr.setTransform(x -> new String("Number: " + x));

    String result = intToStr.apply(new Int(42));
    print(result.toString());
}

main();
