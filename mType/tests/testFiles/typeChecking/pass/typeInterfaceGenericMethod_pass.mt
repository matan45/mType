// Test generic interface method implementation with type checking
import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/String.mt";

interface Transformer<T> {
    public function <U> transform(T input, U context): U;
    public function getType(): string;
}

// MYT-360 — generic type arguments must be boxed classes; was `Transformer<string>`.
class StringTransformer implements Transformer<String> {
    public function <U> transform(String input, U context): U {
        print("Transforming: " + input);
        return context;
    }

    public function getType(): string {
        return "StringTransformer";
    }
}

class IntTransformer implements Transformer<Int> {
    public function <U> transform(Int input, U context): U {
        print("Transforming: " + input.getValue());
        return context;
    }

    public function getType(): string {
        return "IntTransformer";
    }
}

StringTransformer strTrans = new StringTransformer();
IntTransformer intTrans = new IntTransformer();

String result1 = strTrans.transform<String>(new String("hello"), new String("world"));
Int result2 = intTrans.transform<Int>(new Int(42), new Int(100));

print(result1.toString());
print(result2.getValue());
print(strTrans.getType());
print(intTrans.getType());
print("Generic interface methods successful");
