// Test: Protected field access from non-subclass
class ClassA {
    protected int protectedValue;

    public ClassA(int val) {
        protectedValue = val;
    }
}

class ClassB {
    public void tryAccess(ClassA obj) {
        print(obj.protectedValue);  // ERROR: Cannot access protected field (not a subclass)
    }
}

ClassA a = new ClassA(10);
ClassB b = new ClassB();
b.tryAccess(a);
