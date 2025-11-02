// Test checked exceptions in lambda bodies type checking error
interface Operation {
    function execute(int x) : int;
}

class CustomException {
    string message;
    constructor(string msg) {
        message = msg;
    }
}

// Lambda throws exception but interface doesn't declare it - should fail
Operation riskyOp = x -> {
    if (x < 0) {
        throw new CustomException("Negative value not allowed");
    }
    return x * 2;
};
