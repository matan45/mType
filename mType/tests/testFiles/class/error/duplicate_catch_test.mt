import "../../lib/exceptions/Exception.mt";

class MyException extends Exception {
    constructor(string msg): super(msg) {}
}

// This should fail to parse - duplicate catch types
try {
    throw new MyException("Test");
} catch (Exception e) {
    print("First catch");
} catch (Exception e2) {  // ERROR: Duplicate catch type
    print("Second catch");
}
