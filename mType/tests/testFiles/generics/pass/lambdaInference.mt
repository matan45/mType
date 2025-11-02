import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/String.mt";

// Lambda type parameter inference
class Mapper<T, R> {
    function(T): R transform;

    public function setTransform(function(T): R t): void {
        transform = t;
    }

    public function map(T input): R {
        return transform(input);
    }
}

function <T, R> applyMapper(Mapper<T, R> mapper, T input): R {
    return mapper.map(input);
}

function main(): void {
    Mapper<Int, String> intMapper = new Mapper<Int, String>();
    intMapper.setTransform(function(Int x): String {
        return new String("Mapped: " + x);
    });

    // Type inference from lambda parameters
    String result = applyMapper(intMapper, new Int(100));
    print(result);
}

main();
