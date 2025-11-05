// Test exception thrown during object construction in async function

import { Int } from "../../lib/primitives/Int.mt";
import { Bool } from "../../lib/primitives/Bool.mt";
import * from "../../lib/exceptions/Exception.mt";

print("=== Async Exception in Constructor Test ===");

class ConstructorException extends Exception {

    public constructor(string r): super(r) {
        
    }

   
}

class FailingObject {
    int value;

    public constructor(Bool shouldFail) {
        if (shouldFail.getValue()) {
            throw new ConstructorException("Constructor failed");
        }
        this.value = 42;
    }

    public function getValue(): int {
        return this.value;
    }
}

class SafeObject {
    int value;

    public constructor(int v) {
        this.value = v;
    }

    public function getValue(): int {
        return this.value;
    }
}

function async createObject(Bool shouldFail): Promise<SafeObject> {
    print("Creating object with shouldFail: " + shouldFail.getValue());

    SafeObject result;
    try {
        FailingObject failing = new FailingObject(shouldFail);
        result = new SafeObject(failing.getValue());
        print("Object created successfully");
    } catch (ConstructorException e) {
        print("Constructor failed: " + e.getMessage());
        result = new SafeObject(999);
    }

    return result;
}

function async main(): Promise<Int> {
    Bool shouldFail = new Bool(true);
    SafeObject obj = await createObject(shouldFail);
    print("Final value: " + obj.getValue());
    return new Int(obj.getValue());
}

main();
