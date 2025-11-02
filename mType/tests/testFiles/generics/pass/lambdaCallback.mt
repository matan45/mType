import * from "../../lib/primitives/Int.mt";
import * from "../../lib/primitives/String.mt";

// Generic callback patterns with lambdas
class AsyncTask<T> {
    T result;
    function(T): void callback;

    public function setCallback(function(T): void cb): void {
        callback = cb;
    }

    public function execute(T res): void {
        result = res;
        callback(result);
    }
}

function main(): void {
    AsyncTask<String> stringTask = new AsyncTask<String>();
    stringTask.setCallback(function(String s): void {
        print("Callback received: " + s);
    });
    stringTask.execute(new String("Success"));

    AsyncTask<Int> intTask = new AsyncTask<Int>();
    intTask.setCallback(function(Int i): void {
        print("Callback received: " + i);
    });
    intTask.execute(new Int(42));
}

main();
