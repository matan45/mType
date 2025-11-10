// Test: Import Object from lib
import { Object } from "../../lib/Object.mt";

class MyClass implements Object<MyClass> {
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
