// Test multiple finally blocks in chain without catch
import * from "../../lib/exceptions/Exception.mt";

function testChainedFinally(): void {
    try {
        print("Outer try");
        try {
            print("Middle try");
            try {
                print("Inner try");
                print("No exceptions thrown");
            } finally {
                print("Inner finally");
            }
        } finally {
            print("Middle finally");
        }
    } finally {
        print("Outer finally");
    }
}

function testChainedFinallyWithException(): void {
    try {
        try {
            print("L1 try");
            try {
                print("L2 try");
                try {
                    print("L3 try");
                    throw new Exception("Deep exception");
                } finally {
                    print("L3 finally");
                }
            } finally {
                print("L2 finally");
            }
        } finally {
            print("L1 finally");
        }
    } catch (Exception e) {
        print("Caught at top: " + e.getMessage());
    }
}

function testChainedFinallyWithReturn(): int {
    try {
        print("Outer try with return");
        try {
            print("Middle try");
            try {
                print("Inner try returning");
                return 42;
            } finally {
                print("Inner finally");
            }
        } finally {
            print("Middle finally");
        }
    } finally {
        print("Outer finally");
    }
}

function testMixedChain(): void {
    try {
        print("Try 1");
        try {
            print("Try 2");
            int x = 10;
        } finally {
            print("Finally 2");
        }
        print("Between try blocks");
        try {
            print("Try 3");
            string s = "test";
        } finally {
            print("Finally 3");
        }
    } finally {
        print("Finally 1");
    }
}

function main(): void {
    print("Testing multiple finally blocks without catch");

    testChainedFinally();
    print("---");

    testChainedFinallyWithException();
    print("---");

    int result = testChainedFinallyWithReturn();
    print("Returned: " + result);
    print("---");

    testMixedChain();

    print("Test completed");
}

main();
