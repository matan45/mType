// Test field modifier introspection

import * from "../../lib/reflect/Class.mt";
import * from "../../lib/reflect/Field.mt";
import * from "../../lib/reflect/Modifier.mt";

class TestClass {
    private string privateField;
    public int publicField;
    protected float protectedField;
    public static int staticField = 0;
    public final int finalField = 42;
    private static final int privateStaticFinal = 100;
}

Class testClass = Class::forName("TestClass");

// Test private field
Field privateField = testClass.getDeclaredField("privateField");
print("privateField is private: " + privateField.isPrivate());
print("privateField is public: " + privateField.isPublic());
print("privateField is static: " + privateField.isStatic());

// Test public field
Field publicField = testClass.getDeclaredField("publicField");
print("publicField is private: " + publicField.isPrivate());
print("publicField is public: " + publicField.isPublic());
print("publicField is static: " + publicField.isStatic());

// Test protected field
Field protectedField = testClass.getDeclaredField("protectedField");
print("protectedField is protected: " + protectedField.isProtected());
print("protectedField is public: " + protectedField.isPublic());

// Test static field
Field staticField = testClass.getDeclaredField("staticField");
print("staticField is static: " + staticField.isStatic());
print("staticField is public: " + staticField.isPublic());

// Test final field
Field finalField = testClass.getDeclaredField("finalField");
print("finalField is final: " + finalField.isFinal());

// Test private static final
Field psfField = testClass.getDeclaredField("privateStaticFinal");
print("privateStaticFinal is private: " + psfField.isPrivate());
print("privateStaticFinal is static: " + psfField.isStatic());
print("privateStaticFinal is final: " + psfField.isFinal());

print("Test passed");
