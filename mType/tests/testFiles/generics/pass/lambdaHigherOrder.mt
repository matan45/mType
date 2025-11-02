import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/String.mt";

// Higher-order generic functions with lambdas
class Transform<T, R> {
    function(function(T): R, T): R applicator;

    public function setApplicator(function(function(T): R, T): R app): void {
        applicator = app;
    }

    public function apply(function(T): R fn, T value): R {
        return applicator(fn, value);
    }
}

function main(): void {
    Transform<Int, String> transformer = new Transform<Int, String>();
    transformer.setApplicator(function(function(Int): String fn, Int val): String {
        return fn(val);
    });

    String result = transformer.apply(
        function(Int x): String {
            return new String("Transformed: " + x);
        },
        new Int(99)
    );

    print(result);
}

main();
