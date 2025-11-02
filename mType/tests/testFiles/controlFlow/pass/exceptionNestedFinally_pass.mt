// Test multiple nested finally blocks
import * from "../../lib/exceptions/Exception.mt";

function testNestedFinally(): void {
    print("Testing nested finally blocks");

    try {
        print("Outer try");
        try {
            print("Middle try");
            try {
                print("Inner try");
                throw new Exception("Deep exception");
            } finally {
                print("Inner finally");
            }
        } finally {
            print("Middle finally");
        }
    } catch (Exception e) {
        print("Outer catch: " + e.getMessage());
    } finally {
        print("Outer finally");
    }
}

function testNestedFinallyWithReturns(): int {
    try {
        print("Level 1 try");
        try {
            print("Level 2 try");
            try {
                print("Level 3 try");
                return 1;
            } finally {
                print("Level 3 finally");
            }
        } finally {
            print("Level 2 finally");
        }
    } finally {
        print("Level 1 finally");
    }
}

function testNestedFinallyWithBreak(): void {
    print("Testing nested finally with break");

    for (int i = 0; i < 2; i++) {
        try {
            print("Outer loop try: " + i);
            for (int j = 0; j < 3; j++) {
                try {
                    print("Inner loop try: j=" + j);
                    if (j == 1) {
                        break;
                    }
                } finally {
                    print("Inner loop finally: j=" + j);
                }
            }
        } finally {
            print("Outer loop finally: i=" + i);
        }
    }
}

function testComplexNesting(): void {
    print("Testing complex nesting");

    try {
        print("L1 try");
        try {
            print("L2 try");
            throw new Exception("L2 exception");
        } catch (Exception e) {
            print("L2 catch: " + e.getMessage());
            try {
                print("L3 try in catch");
                throw new Exception("L3 exception");
            } finally {
                print("L3 finally");
            }
        } finally {
            print("L2 finally");
        }
    } catch (Exception e) {
        print("L1 catch: " + e.getMessage());
    } finally {
        print("L1 finally");
    }
}

function main(): void {
    print("Testing nested finally blocks");

    testNestedFinally();

    int result = testNestedFinallyWithReturns();
    print("Result: " + result);

    testNestedFinallyWithBreak();
    testComplexNesting();

    print("Test completed");
}

main();
