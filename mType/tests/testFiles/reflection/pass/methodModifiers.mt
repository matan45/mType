// Test method modifier introspection

import * from "../../lib/reflect/Class.mt";
import * from "../../lib/reflect/Method.mt";
import * from "../../lib/reflect/Modifier.mt";

class TestClass {
    public function publicMethod(): void {}
    private function privateMethod(): void {}
    protected function protectedMethod(): void {}
    public static function staticMethod(): void {}
    public final function finalMethod(): void {}
}

Class testClass = Class::forName("TestClass");

// Test public method
Method publicMethod = testClass.getDeclaredMethod("publicMethod", 0);
print("publicMethod is public: " + publicMethod.isPublic());
print("publicMethod is private: " + publicMethod.isPrivate());

// Test private method
Method privateMethod = testClass.getDeclaredMethod("privateMethod", 0);
print("privateMethod is public: " + privateMethod.isPublic());
print("privateMethod is private: " + privateMethod.isPrivate());

// Test protected method
Method protectedMethod = testClass.getDeclaredMethod("protectedMethod", 0);
print("protectedMethod is protected: " + protectedMethod.isProtected());

// Test static method
Method staticMethod = testClass.getDeclaredMethod("staticMethod", 0);
print("staticMethod is static: " + staticMethod.isStatic());
print("staticMethod is public: " + staticMethod.isPublic());

// Test final method
Method finalMethod = testClass.getDeclaredMethod("finalMethod", 0);
print("finalMethod is final: " + finalMethod.isFinal());

print("Test passed");
