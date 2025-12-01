// Test field type introspection

import * from "../../lib/reflect/Class.mt";
import * from "../../lib/reflect/Field.mt";

class OtherClass {
    public int value;
}

class TestClass {
    public int intField;
    public float floatField;
    public bool boolField;
    public string stringField;
    public OtherClass objectField;
    public int[] arrayField;
}

Class testClass = Class::forName("TestClass");

// Get all fields and check their types
Field intField = testClass.getDeclaredField("intField");
print("intField type: " + intField.getType());

Field floatField = testClass.getDeclaredField("floatField");
print("floatField type: " + floatField.getType());

Field boolField = testClass.getDeclaredField("boolField");
print("boolField type: " + boolField.getType());

Field stringField = testClass.getDeclaredField("stringField");
print("stringField type: " + stringField.getType());

Field objectField = testClass.getDeclaredField("objectField");
print("objectField type: " + objectField.getType());

Field arrayField = testClass.getDeclaredField("arrayField");
print("arrayField type: " + arrayField.getType());

print("Test passed");
