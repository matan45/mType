// Test: Implicit Object inheritance
// Expected: Pass - all classes automatically inherit toString, equals, hashCode from Object

class MyClass {
    public function test(): string {
        return "Inherits from Object";
    }

    public function toString(): string {
        return "MyClass instance";
    }

    public function equals(MyClass other): bool {
        return true;
    }

    public function hashCode(): int {
        return 42;
    }
}

MyClass obj = new MyClass();
print(obj.test());
