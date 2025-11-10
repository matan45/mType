import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/String.mt";

// Lambda type parameter inference
interface Transform<T, R> {
    function apply(T input): R;
}

class Mapper<T, R> {
    Transform<T, R> transform;

    public function setTransform(Transform<T, R> t): void {
        transform = t;
    }

    public function map(T input): R {
        return transform.apply(input);
    }
}

function <T, R> applyMapper(Mapper<T, R> mapper, T input): R {
    return mapper.map(input);
}

function main(): void {
    Mapper<Int, String> intMapper = new Mapper<Int, String>();
    intMapper.setTransform(x -> new String("Mapped: " + x));

    // Type inference from lambda parameters
    String result = applyMapper<Int, String>(intMapper, new Int(100));
    print(result.toString());
}

main();
