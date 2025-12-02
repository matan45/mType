// Test field discovery via reflection

import * from "../../lib/reflect/Class.mt";
import * from "../../lib/reflect/Field.mt";

class TestClass {
    private string name;
    public int age;
    protected float score;
    public static int count = 0;
}

Class testClass = Class::forName("TestClass");

// Get all declared fields
Field[] fields = testClass.getDeclaredFields();
print("Number of declared fields: " + fields.length);

// Print each field's name and type
for (Field f : fields) {
    print("Field: " + f.getName() + " type=" + f.getType());
}

// Get specific field by name
Field ageField = testClass.getDeclaredField("age");
print("Found field 'age': " + ageField.getName());

Field countField = testClass.getDeclaredField("count");
print("Found field 'count': " + countField.getName());

print("Test passed");
