import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/String.mt";
import * from "../../lib/primitives/Bool.mt";

// Generic exception types
interface Supplier<T> {
    function get(): T;
}

class Result<T> {
    T value;
    Bool hasError;
    String errorMessage;

    public function setSuccess(T v): void {
        value = v;
        hasError = new Bool(false);
    }

    public function setError(String msg): void {
        errorMessage = msg;
        hasError = new Bool(true);
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

class Operation {
    public static function <R> tryCatch(Supplier<R> operation): Result<R> {
        Result<R> result = new Result<R>();
        R value = operation.get();
        result.setSuccess(value);
        return result;
    }
}

function main(): void {
    Result<Int> intResult = Operation::tryCatch<Int>(() -> new Int(42));
    if (!intResult.isError().getValue()) {
        print("Int result: " + intResult.getValue().toString());
    }

    Result<String> strResult = Operation::tryCatch<String>(() -> new String("Success"));
    if (!strResult.isError().getValue()) {
        print("String result: " + strResult.getValue().toString());
    }
}

main();
