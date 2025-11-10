import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/String.mt";

// Higher-order generic functions with lambdas
interface Func<T, R> {
    function apply(T input): R;
}

interface Applicator<T, R> {
    function apply(Func<T, R> fn, T value): R;
}

class Transform<T, R> {
    Applicator<T, R> applicator;

    public function setApplicator(Applicator<T, R> app): void {
        applicator = app;
    }

    public function apply(Func<T, R> fn, T value): R {
        return applicator.apply(fn, value);
    }
}

function main(): void {
    Transform<Int, String> transformer = new Transform<Int, String>();
    transformer.setApplicator((fn, val) -> fn.apply(val));

    String result = transformer.apply(
        x -> new String("Transformed: " + x),
        new Int(99)
    );

    print(result.toString());
}

main();
