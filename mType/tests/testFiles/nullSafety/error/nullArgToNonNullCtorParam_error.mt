// Test: Passing null to a non-nullable constructor parameter should fail at compile time

class Wrapper {
    public Object inner;
    constructor(Object obj) {
        this.inner = obj;
    }
}

Wrapper w = new Wrapper(null);
print("Should not reach here");
