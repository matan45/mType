import * from "../../lib/exceptions/Exception.mt";

class MyError extends Exception {
    public constructor(string msg) : super(msg) {
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
    print(ex.getMessage());
}
print("after");
