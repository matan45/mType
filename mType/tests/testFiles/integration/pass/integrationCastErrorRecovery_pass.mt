// Test: Error recovery after invalid cast operations
// Expected: Pass - demonstrates recovery mechanisms after cast failures

import * from "../../lib/exceptions/Exception.mt";

class CastException extends Exception {
    constructor(string msg):super(msg) {
    }
}

class RecoveryException extends Exception {
    constructor(string msg):super(msg) {
    }
}

class Component {
    protected string name;
    protected bool isActive;

    public constructor(string name) {
        this.name = name;
        this.isActive = true;
    }

    public string getName() {
        return this.name;
    }

    public void activate() {
        this.isActive = true;
        print("Activated: " + this.name);
    }

    public void deactivate() {
        this.isActive = false;
        print("Deactivated: " + this.name);
    }

    public bool getIsActive() {
        return this.isActive;
    }
}

class Button extends Component {
    private string label;

    public constructor(string name, string label) : super(name) {
        this.label = label;
    }

    public string getLabel() {
        return this.label;
    }

    public void click() {
        print("Button clicked: " + this.label);
    }
}

class TextBox extends Component {
    private string value;

    public constructor(string name, string value) : super(name) {
        this.value = value;
    }

    public string getValue() {
        return this.value;
    }

    public void setValue(string v) {
        this.value = v;
        print("TextBox value set: " + v);
    }
}

// Test 1: Recovery with fallback type
function castWithFallback(Component c): string {
    try {
        if (c instanceof Button) {
            Button b = (Button)c;
            b.click();
            return "Button action performed";
        } else {
            throw new CastException("Not a button");
        }
    } catch (CastException e) {
        print("Cast failed: " + e.getMessage());
        print("Attempting fallback to TextBox");

        if (c instanceof TextBox) {
            TextBox t = (TextBox)c;
            print("Fallback successful: " + t.getValue());
            return "Recovered with TextBox";
        } else {
            print("Fallback failed, using base Component");
            c.activate();
            return "Recovered with base type";
        }
    }
}

// Test 2: Recovery with retry mechanism
function castWithRetry(Component c, int maxAttempts): string {
    int attempt = 0;

    while (attempt < maxAttempts) {
        attempt = attempt + 1;
        print("Attempt " + attempt + " of " + maxAttempts);

        try {
            if (c instanceof Button) {
                Button b = (Button)c;
                b.click();
                return "Success on attempt " + attempt;
            } else {
                throw new CastException("Not a button, retrying");
            }
        } catch (CastException e) {
            print("Failed: " + e.getMessage());
            if (attempt >= maxAttempts) {
                print("Max attempts reached, recovering");
                c.activate();
                return "Recovered after " + attempt + " attempts";
            }
        }
    }

    return "Recovery loop completed";
}

// Test 3: Recovery with state restoration
function castWithStateRestore(Component c): string {
    bool originalState = c.getIsActive();
    print("Original state: " + originalState);

    try {
        c.deactivate();

        if (c instanceof Button) {
            Button b = (Button)b;
            b.click();
            return "Button processed";
        } else {
            throw new CastException("Invalid type");
        }
    } catch (CastException e) {
        print("Error occurred: " + e.getMessage());
        print("Restoring original state");

        if (originalState) {
            c.activate();
        }

        return "State restored after error";
    }
}

// Test 4: Recovery with multiple fallback options
function castWithMultipleFallbacks(Component c): string {
    try {
        if (c instanceof Button) {
            Button b = (Button)c;
            b.click();
            throw new RecoveryException("Simulated error after cast");
        } else {
            throw new CastException("Primary cast failed");
        }
    } catch (CastException e) {
        print("Primary failed: " + e.getMessage());
        try {
            if (c instanceof TextBox) {
                TextBox t = (TextBox)c;
                t.setValue("recovered");
                return "Recovered with TextBox";
            } else {
                throw new CastException("Secondary cast failed");
            }
        } catch (CastException e2) {
            print("Secondary failed: " + e2.getMessage());
            print("Using tertiary recovery");
            c.activate();
            return "Recovered with base Component";
        }
    } catch (RecoveryException e) {
        print("Recovery exception: " + e.getMessage());
        return "Handled recovery exception";
    }
}

// Test 5: Recovery from exception in cast result usage
function castWithUsageRecovery(Component c): string {
    try {
        if (c instanceof TextBox) {
            TextBox t = (TextBox)c;
            string val = t.getValue();

            if (val == "error") {
                throw new RecoveryException("Invalid value in TextBox");
            }

            t.setValue("processed");
            return "TextBox processed successfully";
        } else {
            throw new CastException("Not a TextBox");
        }
    } catch (RecoveryException e) {
        print("Usage error: " + e.getMessage());
        print("Recovering by resetting component");
        c.deactivate();
        c.activate();
        return "Recovered from usage error";
    } catch (CastException e) {
        print("Cast error: " + e.getMessage());
        if (c instanceof Button) {
            Button b = (Button)c;
            b.click();
            return "Recovered with Button";
        }
        return "Could not recover";
    }
}

// Test 6: Batch processing with error recovery
function batchCastWithRecovery(Component[] components): string {
    int successful = 0;
    int recovered = 0;
    int failed = 0;

    int i = 0;
    while (i < 3) {
        try {
            if (components[i] instanceof Button) {
                Button b = (Button)components[i];
                b.click();
                successful = successful + 1;
                print("Item " + i + ": Success");
            } else {
                throw new CastException("Item " + i + " not a Button");
            }
        } catch (CastException e) {
            print(e.getMessage());
            try {
                if (components[i] instanceof TextBox) {
                    TextBox t = (TextBox)components[i];
                    print("Recovered: Using TextBox " + t.getValue());
                    recovered = recovered + 1;
                } else {
                    print("Recovery failed, using base");
                    components[i].activate();
                    recovered = recovered + 1;
                }
            } catch (Exception e2) {
                print("Recovery failed completely");
                failed = failed + 1;
            }
        }
        i = i + 1;
    }

    return "Success: " + successful + ", Recovered: " + recovered + ", Failed: " + failed;
}

// Run all tests
print("=== Test 1: Cast with fallback (Button) ===");
Component btn1 = new Button("btn1", "Submit");
print(castWithFallback(btn1));

print("\n=== Test 1b: Cast with fallback (TextBox) ===");
Component txt1 = new TextBox("txt1", "Hello");
print(castWithFallback(txt1));

print("\n=== Test 1c: Cast with fallback (Base) ===");
Component comp1 = new Component("comp1");
print(castWithFallback(comp1));

print("\n=== Test 2: Cast with retry (Button) ===");
Component btn2 = new Button("btn2", "Click Me");
print(castWithRetry(btn2, 3));

print("\n=== Test 2b: Cast with retry (TextBox) ===");
Component txt2 = new TextBox("txt2", "Data");
print(castWithRetry(txt2, 2));

print("\n=== Test 3: Cast with state restore ===");
Component txt3 = new TextBox("txt3", "State Test");
print(castWithStateRestore(txt3));

print("\n=== Test 4: Multiple fallbacks (Button with error) ===");
Component btn3 = new Button("btn3", "Error Button");
print(castWithMultipleFallbacks(btn3));

print("\n=== Test 4b: Multiple fallbacks (TextBox) ===");
Component txt4 = new TextBox("txt4", "Fallback");
print(castWithMultipleFallbacks(txt4));

print("\n=== Test 4c: Multiple fallbacks (Base) ===");
Component comp2 = new Component("comp2");
print(castWithMultipleFallbacks(comp2));

print("\n=== Test 5: Usage recovery (valid TextBox) ===");
Component txt5 = new TextBox("txt5", "valid");
print(castWithUsageRecovery(txt5));

print("\n=== Test 5b: Usage recovery (error TextBox) ===");
Component txt6 = new TextBox("txt6", "error");
print(castWithUsageRecovery(txt6));

print("\n=== Test 5c: Usage recovery (Button fallback) ===");
Component btn4 = new Button("btn4", "Fallback Button");
print(castWithUsageRecovery(btn4));

print("\n=== Test 6: Batch processing with recovery ===");
Component[] arr = new Component[3];
arr[0] = new Button("btn5", "First");
arr[1] = new TextBox("txt7", "Second");
arr[2] = new Button("btn6", "Third");
print(batchCastWithRecovery(arr));

print("\nAll error recovery tests completed");
