import * from "../../lib/primitives/String.mt";
import * from "../../lib/primitives/Int.mt";

// Test: Class-level and method-level generic parameters together
class Box<E> {
    E data;

    public constructor(E value) {
        this.data = value;
    }

    // Instance method with its own generic type parameter (U)
    // Class has E, method has U - both should be accessible
    public function <U> transform(U input): U {
        print("Box contains: " + this.data);
        print("Transform input: " + input);
        return input;
    }

    // Instance method using only class-level generic E
    public function getData(): E {
        return this.data;
    }

    // Instance method with generic returning class-level type
    public function <U> process(U input): E {
        print("Processing: " + input);
        return this.data;
    }
}

function main(): void {
    // Create Box<Int>
    Box<Int> intBox = new Box<Int>(new Int(42));

    // Call method with method-level generic U=String
    String transformResult = intBox.transform<String>(new String("test"));
    print("Transform result: " + transformResult);

    // Call method using only class-level generic E=Int
    Int getData = intBox.getData();
    print("GetData result: " + getData);

    // Call method with U=String, returns E=Int
    Int processResult = intBox.process<String>(new String("input"));
    print("Process result: " + processResult);

    print("Mixed class and method generics test passed");
}

main();
