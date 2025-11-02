import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/String.mt";

// Generic exception types
class Result<T> {
    T value;
    Bool hasError;
    String errorMessage;

    public function setSuccess(T v): void {
        value = v;
        hasError = false;
    }

    public function setError(String msg): void {
        errorMessage = msg;
        hasError = true;
    }

    public function isError(): Bool {
        return hasError;
    }

    public function getValue(): T {
        return value;
    }

    public function getError(): String {
        return errorMessage;
    }
}

class Operation<T> {
    public static function <T> tryCatch(function(): T operation): Result<T> {
        Result<T> result = new Result<T>();
        try {
            T value = operation();
            result.setSuccess(value);
        } catch (Exception e) {
            result.setError(new String("Error occurred"));
        }
        return result;
    }
}

function riskyInt(): Int {
    return new Int(42);
}

function riskyString(): String {
    return new String("Success");
}

function main(): void {
    Result<Int> intResult = Operation.tryCatch(riskyInt);
    if (!intResult.isError()) {
        print("Int result: " + intResult.getValue());
    }

    Result<String> strResult = Operation.tryCatch(riskyString);
    if (!strResult.isError()) {
        print("String result: " + strResult.getValue());
    }
}

main();
