import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/String.mt";

// Generic callback patterns with lambdas
interface Callback<T> {
    function onComplete(T result): void;
}

class AsyncTask<T> {
    T result;
    Callback<T> callback;

    public function setCallback(Callback<T> cb): void {
        callback = cb;
    }

    public function execute(T res): void {
        result = res;
        callback.onComplete(result);
    }
}

function main(): void {
    AsyncTask<String> stringTask = new AsyncTask<String>();
    stringTask.setCallback(s -> print("Callback received: " + s));
    stringTask.execute(new String("Success"));

    AsyncTask<Int> intTask = new AsyncTask<Int>();
    intTask.setCallback(i -> print("Callback received: " + i));
    intTask.execute(new Int(42));
}

main();
