// Test generics with imports - instantiate classes from other files
import * from "genericUtils.mt";
import * from "../../lib/primitives/Bool.mt";
import * from "../../lib/primitives/String.mt";
import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/Float.mt";

function main(): void {
    // Test Wrapper<Bool> - using default constructor and methods
    Wrapper<Bool> boolWrapper = new Wrapper<Bool>();
    boolWrapper.wrap(new Bool(true));
    print("Bool wrapper value: " + boolWrapper.unwrap());
    print("Bool wrapper string: " + boolWrapper.toString());

    // Test Wrapper<String>
    Wrapper<String> stringWrapper = new Wrapper<String>();
    stringWrapper.wrap(new String("Hello"));
    print("String wrapper value: " + stringWrapper.unwrap());
    print("String wrapper string: " + stringWrapper.toString());

    // Test Wrapper<Int>
    Wrapper<Int> intWrapper = new Wrapper<Int>();
    intWrapper.wrap(new Int(42));
    print("Int wrapper value: " + intWrapper.unwrap());
    print("Int wrapper string: " + intWrapper.toString());

    // Test Wrapper<Float>
    Wrapper<Float> floatWrapper = new Wrapper<Float>();
    floatWrapper.wrap(new Float(3.14));
    print("Float wrapper value: " + floatWrapper.unwrap());
    print("Float wrapper string: " + floatWrapper.toString());
}

main();