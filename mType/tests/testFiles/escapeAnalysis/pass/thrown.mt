class MyError {
    public string message;
    public constructor(string m) {
        this.message = m;
    }
}

function mayThrow(bool should): int {
    if (should) {
        MyError e = new MyError("boom");
        throw e;
    }
    return 7;
}

try {
    mayThrow(true);
} catch (MyError ex) {
    print(ex.message);
}
print("after");
