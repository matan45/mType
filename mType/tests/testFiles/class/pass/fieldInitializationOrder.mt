// Test: Field initialization order in classes
// Expected: Pass - demonstrates field initialization order

class InitOrder {
    // Fields should be initialized in declaration order
    public int first = this.initField("first", 1);
    public int second = this.initField("second", 2);
    public int third = this.initField("third", 3);
    public int fourth;

    public constructor() {
        print("Constructor start");
        this.fourth = this.initField("fourth", 4);
        print("Constructor end");
    }

    public function initField(string name,int value): int {
        print("Initializing field: " + name + " = " + value);
        return value;
    }

    public function display(): void {
        print("first=" + this.first + ", second=" + this.second +
              ", third=" + this.third + ", fourth=" + this.fourth);
    }
}

class DerivedInit extends InitOrder {
    public int fifth = this.initField("fifth", 5);

    public constructor() : super() {
        print("DerivedInit constructor");
    }
}

// Test field initialization order
print("Creating InitOrder:");
InitOrder obj1 = new InitOrder();
obj1.display();

print("\nCreating DerivedInit:");
DerivedInit obj2 = new DerivedInit();
obj2.display();
