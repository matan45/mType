// Test generics with imports - instantiate classes from other files
import "genericUtils.mt";

function main(): void {
    // Test Wrapper<bool> - using default constructor and methods
    Wrapper<bool> boolWrapper = new Wrapper<bool>();
    boolWrapper.wrap(true);
    print("Bool wrapper value: " + boolWrapper.unwrap());
    print("Bool wrapper string: " + boolWrapper.toString());

    // Test Wrapper<string>
    Wrapper<string> stringWrapper = new Wrapper<string>();
    stringWrapper.wrap("Hello");
    print("String wrapper value: " + stringWrapper.unwrap());
    print("String wrapper string: " + stringWrapper.toString());

    // Test Wrapper<int>
    Wrapper<int> intWrapper = new Wrapper<int>();
    intWrapper.wrap(42);
    print("Int wrapper value: " + intWrapper.unwrap());
    print("Int wrapper string: " + intWrapper.toString());

    // Test Wrapper<float>
    Wrapper<float> floatWrapper = new Wrapper<float>();
    floatWrapper.wrap(3.14);
    print("Float wrapper value: " + floatWrapper.unwrap());
    print("Float wrapper string: " + floatWrapper.toString());
}

main();