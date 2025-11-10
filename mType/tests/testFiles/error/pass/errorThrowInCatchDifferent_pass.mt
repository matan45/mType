// Test throw in catch block with finally throwing different exception
import * from "../../lib/exceptions/Exception.mt";

class FirstException extends Exception {
    constructor(string message): super(message) {
    }
}

class SecondException extends Exception {
    constructor(string message): super(message) {
    }
}

class ThirdException extends Exception {
    constructor(string message): super(message) {
    }
}

function testCatchThrowFinallyThrow(): void {
    try {
        try {
            print("Try: throwing FirstException");
            throw new FirstException("Original error");
        } catch (FirstException e) {
            print("Catch: caught FirstException, throwing SecondException");
            throw new SecondException("Error from catch");
        } finally {
            print("Finally: throwing ThirdException");
            throw new ThirdException("Error from finally");
        }
    } catch (ThirdException e) {
        print("Outer catch ThirdException: " + e.getMessage());
    } catch (SecondException e) {
        print("Outer catch SecondException: " + e.getMessage());
    }
}

function testFinallyOverridesCatchThrow(): void {
    try {
        try {
            print("Throwing initial exception");
            throw new FirstException("Initial");
        } catch (FirstException e) {
            print("Catch block throwing SecondException");
            throw new SecondException("From catch: " + e.getMessage());
        } finally {
            print("Finally overriding with ThirdException");
            throw new ThirdException("Finally wins");
        }
    } catch (Exception e) {
        print("Final catch: " + e.getMessage());
    }
}

function testNestedCatchThrowFinallyThrow(): void {
    try {
        try {
            try {
                print("L3: throwing FirstException");
                throw new FirstException("Level 3");
            } catch (FirstException e) {
                print("L2 catch: throwing SecondException");
                throw new SecondException("Level 2");
            } finally {
                print("L2 finally: throwing ThirdException");
                throw new ThirdException("Level 2 finally");
            }
        } catch (ThirdException e) {
            print("L1 catch ThirdException: " + e.getMessage());
            print("L1 catch: throwing FirstException");
            throw new FirstException("Level 1 catch");
        } finally {
            print("L1 finally: throwing SecondException");
            throw new SecondException("Level 1 finally");
        }
    } catch (Exception e) {
        print("Top level: " + e.getMessage());
    }
}

function testFinallyThrowWithoutCatchThrow(): void {
    try {
        try {
            print("Try block - no exception");
            int x = 10;
        } catch (Exception e) {
            print("Catch - should not execute");
        } finally {
            print("Finally - throwing exception");
            throw new FirstException("From finally alone");
        }
    } catch (FirstException e) {
        print("Caught from finally: " + e.getMessage());
    }
}

function testCatchThrowWithoutFinallyThrow(): void {
    try {
        try {
            print("Throwing exception");
            throw new FirstException("Original");
        } catch (FirstException e) {
            print("Catch throwing different exception");
            throw new SecondException("From catch");
        } finally {
            print("Finally - no throw");
        }
    } catch (SecondException e) {
        print("Caught from catch: " + e.getMessage());
    }
}

function main(): void {
    print("Testing throw in catch with finally throwing different exception");

    testCatchThrowFinallyThrow();
    print("---");

    testFinallyOverridesCatchThrow();
    print("---");

    testNestedCatchThrowFinallyThrow();
    print("---");

    testFinallyThrowWithoutCatchThrow();
    print("---");

    testCatchThrowWithoutFinallyThrow();

    print("Test completed");
}

main();
