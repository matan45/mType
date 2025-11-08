// Lambda accessing protected members test
interface Function {
    function apply(int x) : int;
}

class Base {
    protected int protectedValue;

    constructor(int val) {
        this.protectedValue = val;
    }

    public function getProtected() : int {
        return this.protectedValue;
    }
}

class Derived extends Base {
    constructor(int val):super(val) {
    
    }

    public function createProtectedAccessor() : Function {
        // Lambda accessing protected member from parent
        Function accessor = x -> {
            return this.protectedValue + x;
        };
        return accessor;
    }

    public function createProtectedModifier() : Function {
        Function modifier = x -> {
            this.protectedValue = this.protectedValue * x;
            return this.protectedValue;
        };
        return modifier;
    }
}

print("=== Protected Access Test ===");

Derived obj = new Derived(10);

Function accessor = obj.createProtectedAccessor();
print("Protected + 5: " + accessor.apply(5));
print("Protected + 10: " + accessor.apply(10));

Function modifier = obj.createProtectedModifier();
print("Multiply by 2: " + modifier.apply(2));
print("Multiply by 3: " + modifier.apply(3));
print("Final protected: " + obj.getProtected());

print("Protected access complete");
