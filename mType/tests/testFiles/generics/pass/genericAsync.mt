import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/String.mt";
import * from "../../lib/primitives/Bool.mt";

// Generic async/await patterns
interface Operation<T> {
    function execute(): T;
}

class Promise<T> {
    T result;
    Bool completed;

    public constructor() {
        completed = new Bool(false);
    }

    public function resolve(T value): void {
        result = value;
        completed = new Bool(true);
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

    public constructor() {
        promise = new Promise<T>();
    }

    public function execute(Operation<T> operation): Promise<T> {
        T result = operation.execute();
        promise.resolve(result);
        return promise;
    }
}

function main(): void {
    AsyncOperation<Int> intOp = new AsyncOperation<Int>();
    Promise<Int> intPromise = intOp.execute(() -> new Int(100));
    if (intPromise.isCompleted()) {
        print("Int: " + intPromise.getResult());
    }

    AsyncOperation<String> strOp = new AsyncOperation<String>();
    Promise<String> strPromise = strOp.execute(() -> new String("Async data"));
    if (strPromise.isCompleted()) {
        print("String: " + strPromise.getResult());
    }
}

main();
