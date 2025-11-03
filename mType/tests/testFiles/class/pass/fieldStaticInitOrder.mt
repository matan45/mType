// Test: Static field initialization order
// Expected: Pass - demonstrates static field initialization order

class StaticInit {
    public static int counter = 0;

    public static int first = StaticInit::initStatic("first", 10);
    public static int second = StaticInit::initStatic("second", 20);
    public static int third = StaticInit::initStatic("third", 30);

    public static function initStatic(string name,int value): int {
        counter++;
        print("Static init " + counter + ": " + name + " = " + value);
        return value;
    }

    public static function displayStatics(): void {
        print("first=" + first + ", second=" + second +
              ", third=" + third + ", counter=" + counter);
    }
}

class DerivedStaticInit extends StaticInit {
    public static int fourth = StaticInit::initStatic("fourth", 40);
}

// Test static field initialization
print("Accessing StaticInit class:");
StaticInit::displayStatics();

print("\nAccessing DerivedStaticInit:");
print("fourth=" + DerivedStaticInit::fourth);
StaticInit::displayStatics();
