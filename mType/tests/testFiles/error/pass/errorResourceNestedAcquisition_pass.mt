// Test: Nested resource acquisition with exception
// Expected: Resources should be released in reverse order
import * from "../../lib/exceptions/Exception.mt";

class AcquisitionException extends Exception {
    constructor(string message): super(message) {
    }
}

class ResourceHandle {
    public string resourceId;
    public int level;
    public bool acquired;

    public constructor(string id, int acquisitionLevel) {
        resourceId = id;
        level = acquisitionLevel;
        acquired = false;
    }

    public function acquire(): void {
        print("Level " + level + ": Acquiring " + resourceId);
        acquired = true;
    }

    public function release(): void {
        if (acquired) {
            print("Level " + level + ": Releasing " + resourceId);
            acquired = false;
        }
    }
}

function testNestedAcquisitionSuccess(): void {
    print("Test 1: Successful nested acquisition and release");
    ResourceHandle? r1 = null;
    ResourceHandle? r2 = null;
    ResourceHandle? r3 = null;

    try {
        r1 = new ResourceHandle("Lock_A", 1);
        r1.acquire();

        try {
            r2 = new ResourceHandle("Lock_B", 2);
            r2.acquire();

            try {
                r3 = new ResourceHandle("Lock_C", 3);
                r3.acquire();

                print("All resources acquired - performing work");
            } finally {
                if (r3 != null) {
                    r3.release();
                }
            }
        } finally {
            if (r2 != null) {
                r2.release();
            }
        }
    } finally {
        if (r1 != null) {
            r1.release();
        }
    }
    print("All resources released in reverse order");
}

function testNestedAcquisitionWithException(): void {
    print("Test 2: Exception during nested acquisition");
    ResourceHandle? r1 = null;
    ResourceHandle? r2 = null;
    ResourceHandle? r3 = null;

    try {
        r1 = new ResourceHandle("Database", 1);
        r1.acquire();

        try {
            r2 = new ResourceHandle("Connection", 2);
            r2.acquire();

            try {
                r3 = new ResourceHandle("Transaction", 3);
                r3.acquire();

                print("Simulating error during transaction");
                throw new AcquisitionException("Transaction validation failed");
            } catch (AcquisitionException e) {
                print("Caught at level 3: " + e.getMessage());
            } finally {
                if (r3 != null) {
                    r3.release();
                }
            }

            print("Level 2 cleanup continuing");
        } finally {
            if (r2 != null) {
                r2.release();
            }
        }

        print("Level 1 cleanup continuing");
    } finally {
        if (r1 != null) {
            r1.release();
        }
    }
    print("Exception handled, all resources released");
}

function testPartialAcquisitionFailure(): void {
    print("Test 3: Failure during middle acquisition");
    ResourceHandle? r1 = null;
    ResourceHandle? r2 = null;
    ResourceHandle? r3 = null;

    try {
        r1 = new ResourceHandle("Resource_1", 1);
        r1.acquire();

        try {
            r2 = new ResourceHandle("Resource_2", 2);
            throw new AcquisitionException("Failed to acquire Resource_2");
            r2.acquire();

            try {
                r3 = new ResourceHandle("Resource_3", 3);
                r3.acquire();
            } finally {
                if (r3 != null) {
                    r3.release();
                }
            }
        } catch (AcquisitionException e) {
            print("Acquisition failed at level 2: " + e.getMessage());
        } finally {
            if (r2 != null) {
                r2.release();
            }
        }
    } finally {
        if (r1 != null) {
            r1.release();
        }
    }
    print("Partial acquisition cleaned up");
}

function testDeepNesting(): void {
    print("Test 4: Deep nesting with exception");
    ResourceHandle? r1 = null;
    ResourceHandle? r2 = null;
    ResourceHandle? r3 = null;
    ResourceHandle? r4 = null;
    ResourceHandle? r5 = null;

    try {
        r1 = new ResourceHandle("L1", 1);
        r1.acquire();

        try {
            r2 = new ResourceHandle("L2", 2);
            r2.acquire();

            try {
                r3 = new ResourceHandle("L3", 3);
                r3.acquire();

                try {
                    r4 = new ResourceHandle("L4", 4);
                    r4.acquire();

                    try {
                        r5 = new ResourceHandle("L5", 5);
                        r5.acquire();

                        print("Deepest level - throwing exception");
                        throw new Exception("Error at deepest level");
                    } catch (Exception e) {
                        print("Exception at L5: " + e.getMessage());
                    } finally {
                        if (r5 != null) {
                            r5.release();
                        }
                    }
                } finally {
                    if (r4 != null) {
                        r4.release();
                    }
                }
            } finally {
                if (r3 != null) {
                    r3.release();
                }
            }
        } finally {
            if (r2 != null) {
                r2.release();
            }
        }
    } finally {
        if (r1 != null) {
            r1.release();
        }
    }
    print("Deep nesting unwound successfully");
}

function main(): void {
    print("Testing nested resource acquisition with exceptions");
    testNestedAcquisitionSuccess();
    print("---");
    testNestedAcquisitionWithException();
    print("---");
    testPartialAcquisitionFailure();
    print("---");
    testDeepNesting();
    print("Nested acquisition test completed");
}

main();
