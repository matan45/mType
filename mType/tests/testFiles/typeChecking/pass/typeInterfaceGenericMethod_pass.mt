// Test generic interface method implementation with type checking
interface Transformer<T> {
    function transform<U>(T input, U context): U;
    function getType(): string;
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

class IntTransformer implements Transformer<int> {
    public function transform<U>(int input, U context): U {
        print("Transforming: " + input);
        return context;
    }

    public function getType(): string {
        return "IntTransformer";
    }
}

StringTransformer strTrans = new StringTransformer();
IntTransformer intTrans = new IntTransformer();

string result1 = strTrans.transform<string>("hello", "world");
int result2 = intTrans.transform<int>(42, 100);

print(result1);
print(result2);
print(strTrans.getType());
print(intTrans.getType());
print("Generic interface methods successful");
