// MYT-38: Lock in the stack-frame name format for lambdas. Existing lambda tests
// (errorStackTraceWithLambda_pass.mt) only check `trace != ""`, so the actual
// frame string is unverified. Per LambdaExecutor.cpp:148-150, the VM uses
// `lambda->functionName` if non-empty, otherwise falls back to "<lambda>" (or
// "ClassName::<lambda>" if defined inside a class). Either way the substring
// "lambda" must appear. This test pins that — any future change to the lambda
// frame format intentionally breaks it.
import * from "../../lib/exceptions/Exception.mt";

interface Runnable {
    function run(): void;
}

function main(): void {
    Runnable thrower = () -> {
        throw new Exception("lambdaFrameTest");
    };

    try {
        thrower.run();
    } catch (Exception e) {
        string trace = e.getStackTrace();

        int idxLambda = indexOf(trace, "lambda");
        if (idxLambda != -1) { print("contains 'lambda': true"); } else { print("contains 'lambda': false"); }

        if (trace != "") { print("trace non-empty: true"); } else { print("trace non-empty: false"); }
    }
}

main();
