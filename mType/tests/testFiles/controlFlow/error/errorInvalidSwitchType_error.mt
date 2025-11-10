// Test switch on unsupported type - should produce error
// Switch statements typically only support int, string, and enum types

class CustomClass {
    public int value;

    public constructor(int v) {
        value = v;
    }
}

CustomClass obj = new CustomClass(5);

// ERROR: Cannot switch on object type
switch (obj) {
    case new CustomClass(5):
        print("Matches 5");
        break;
    case new CustomClass(10):
        print("Matches 10");
        break;
    default:
        print("No match");
        break;
}

print("This should not execute");
