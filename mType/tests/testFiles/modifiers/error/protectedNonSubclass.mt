// Test: Protected field access from non-subclass
class ClassA {
    protected int protectedValue;

    constructor(int val) {
        protectedValue = val;
    }
}

class ClassB {
    public function tryAccess(ClassA obj): void {
        print(obj.protectedValue);  // ERROR: Cannot access protected field (not a subclass)
    }
}

ClassA a = new ClassA(10);
ClassB b = new ClassB();
b.tryAccess(a);
