// Test type system edge cases

    class TypeContainer {
        int intValue;
        float floatValue;
        string stringValue;
        bool boolValue;
        
        constructor() {
            intValue = 42;
            floatValue = 3.14;
            stringValue = "test";
            boolValue = true;
        }
        
        function typeConversions(): string {
            string result = "";
            
            // Test implicit conversions and operations
            result = result + intValue;
            result = result + " | " + floatValue;
            result = result + " | " + stringValue;
            result = result + " | " + boolValue;
            
            // Test mixed type operations
            float mixedResult = intValue + floatValue;
            result = result + " | Mixed: " + mixedResult;
            
            return result;
        }
        
        function compareTypes(): string {
            string result = "";
            
            if (intValue == 42) {
                result = result + "Int match | ";
            }
            
            if (floatValue > 3.0) {
                result = result + "Float compare | ";
            }
            
            if (stringValue == "test") {
                result = result + "String match | ";
            }
            
            if (boolValue) {
                result = result + "Bool true";
            }
            
            return result;
        }
    }


// Test type system edge cases
TypeContainer container = new TypeContainer();
print(container.typeConversions());
print(container.compareTypes());

// Test type interactions in control flow
if (container.intValue > 0 && container.floatValue > 0.0) {
    print("Both positive");
}

if (container.stringValue == "test" || container.boolValue) {
    print("String or bool condition met");
}