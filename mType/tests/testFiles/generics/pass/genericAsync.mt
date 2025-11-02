import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/String.mt";

// Generic async/await patterns
class Promise<T> {
    T result;
    Bool completed;

    public function Promise() {
        completed = false;
    }

    public function resolve(T value): void {
        result = value;
        completed = true;
    }

    public function isCompleted(): Bool {
        return completed;
    }

    public function getResult(): T {
        return result;
    }
}

class AsyncOperation<T> {
    Promise<T> promise;

    public function AsyncOperation() {
        promise = new Promise<T>();
    }

    public function execute(function(): T operation): Promise<T> {
        T result = operation();
        promise.resolve(result);
        return promise;
    }
}

function fetchInt(): Int {
    return new Int(100);
}

function fetchString(): String {
    return new String("Async data");
}

function main(): void {
    AsyncOperation<Int> intOp = new AsyncOperation<Int>();
    Promise<Int> intPromise = intOp.execute(fetchInt);
    if (intPromise.isCompleted()) {
        print("Int: " + intPromise.getResult());
    }

    AsyncOperation<String> strOp = new AsyncOperation<String>();
    Promise<String> strPromise = strOp.execute(fetchString);
    if (strPromise.isCompleted()) {
        print("String: " + strPromise.getResult());
    }
}

main();
