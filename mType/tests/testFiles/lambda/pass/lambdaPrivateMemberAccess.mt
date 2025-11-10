// Lambda accessing private members test
interface Function {
    function apply(int x) : int;
}

class PrivateExample {
    int privateField;

    constructor(int val) {
        this.privateField = val;
    }

    public function createAccessor() : Function {
        // Lambda can access private fields of its enclosing class
        Function accessor = x -> {
            return this.privateField + x;
        };
        return accessor;
    }

    public function createModifier() : Function {
        Function modifier = x -> {
            this.privateField = this.privateField + x;
            return this.privateField;
        };
        return modifier;
    }

    public function getPrivateField() : int {
        return this.privateField;
    }
}

print("=== Private Member Access Test ===");

PrivateExample obj = new PrivateExample(100);

Function accessor = obj.createAccessor();
print("Access private + 50: " + accessor.apply(50));
print("Access private + 100: " + accessor.apply(100));

Function modifier = obj.createModifier();
print("Modify by 10: " + modifier.apply(10));
print("Modify by 20: " + modifier.apply(20));
print("Final value: " + obj.getPrivateField());

print("Private member access complete");
