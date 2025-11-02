// Test generic interface method implementation with type checking
import * from "../../../lib/primitives/Int.mt";

interface Transformer<T> {
    public function transform<U>(T input, U context): U;
    public function getType(): string;
}

class StringTransformer implements Transformer<string> {
    public function transform<U>(string input, U context): U {
        print("Transforming: " + input);
        return context;
    }

    public function getType(): string {
        return "StringTransformer";
    }
}

class IntTransformer implements Transformer<Int> {
    public function transform<U>(Int input, U context): U {
        print("Transforming: " + input.getValue());
        return context;
    }

    public function getType(): string {
        return "IntTransformer";
    }
}

StringTransformer strTrans = new StringTransformer();
IntTransformer intTrans = new IntTransformer();

string result1 = strTrans.transform<string>("hello", "world");
Int result2 = intTrans.transform<Int>(new Int(42), new Int(100));

print(result1);
print(result2.getValue());
print(strTrans.getType());
print(intTrans.getType());
print("Generic interface methods successful");
